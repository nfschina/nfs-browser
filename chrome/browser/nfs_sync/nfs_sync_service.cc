#include "chrome/browser/nfs_sync/nfs_sync_service.h"

#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/nfs_sync/nfs_theme_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/browser/bookmark_codec.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/nfs_sync/account_checker.h"
#include "components/nfs_sync/nfs_sync_observer.h"
#include "components/nfs_sync/nfs_sync_utils.h"
#include "components/nfs_sync/nfs_sync_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context_getter.h"

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_change.h"
#include "content/public/browser/browser_thread.h"

// using namespace nfs_sync;

namespace {

const char* kURL = "http://124.16.141.197:10001/data";

const char kContentTypeJSON[] = "application/json";
const char kContentHeadersJSON[] =
    "Content-Type: application/json; charset=utf-8";

const char* kNfsSyncThreadName = "Chrome_NfsSyncThread";
const char* kCurrentThemeID = "extensions.theme.id";

std::string BookmarkToString(bookmarks::BookmarkModel* model) {
  DCHECK(model);
  if (!model) {
    return std::string();
  }

  bookmarks::BookmarkCodec codec;
  base::Value* value = codec.Encode(model);
  if (!value) {
    return std::string();
  }

  base::DictionaryValue* dict;
  if (!value->GetAsDictionary(&dict)) {
    return std::string();
  }

  std::string bookmark_data;
  if (!nfs_sync::DictionaryToString(dict, &bookmark_data)) {
    return std::string();
  }

  return bookmark_data;
}

bool BookmarkExist(const int count,
                   const bookmarks::BookmarkNode* node,
                   const base::string16& title,
                   const std::string& url) {
  for (int i = 0; i < count; ++i) {
    const bookmarks::BookmarkNode* child_node = node->GetChild(i);
    if (child_node->is_url() &&
        child_node->GetTitle() == title &&
        child_node->url().spec() == url) {
      return true;
    }
  }
  return false;
}

}

NfsSyncService::NfsSyncService(bool logged_in,
                                 net::URLRequestContextGetter* getter,
                                 Profile* profile)
    : logged_in_(logged_in)
    , bookmark_is_motify_(false)
    , thread_(new base::Thread(kNfsSyncThreadName))
    , profile_(profile)
    , pref_service_(NULL)
    , bookmark_model_(NULL)
    , getter_(getter)
    , account_info_(nfs_sync::AccountInfo()) {
  DCHECK(getter_ && getter_.get());

  account_checker_.reset(nullptr);
  nfs_theme_fetcher_.reset(nullptr);
  nfs_theme_handler_.reset(nullptr);
  nfs_bookmark_fetcher_.reset(nullptr);
  nfs_password_fetecher_.reset(nullptr);
}

NfsSyncService::~NfsSyncService() {
  if (bookmark_model_) {
    bookmark_model_->RemoveObserver(this);
  }
  Cleanup();
}

bool NfsSyncService::Init() {

  DCHECK(profile_);

  if (!profile_) {
    return false;
  }

  bookmark_model_ = BookmarkModelFactory::GetForBrowserContext(profile_);
  pref_service_ = profile_->GetPrefs();
  DCHECK(bookmark_model_ && pref_service_);
  if (!bookmark_model_ || !pref_service_) {
    return false;
  }

  if (!bookmark_model_) {
    return false;
  }

  if (!getter_ || !getter_.get()) {
    return false;
  }

  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  options.timer_slack = base::TIMER_SLACK_MAXIMUM;
  if (!thread_->StartWithOptions(options)) {
    Cleanup();
    return false;
  }

  account_checker_.reset(new nfs_sync::AccountChecker(getter_.get()));
  if (!account_checker_ || !account_checker_.get()) {
    return false;
  }

  nfs_theme_fetcher_.reset(new nfs_sync::NfsThemeFetcher(getter_.get()));
  if (!nfs_theme_fetcher_ || !nfs_theme_fetcher_.get()) {
    return false;
  }

  nfs_password_fetecher_.reset(new nfs_sync::NfsPasswordFetcher(getter_.get()));
  if (!nfs_password_fetecher_ || !nfs_password_fetecher_.get()) {
    return false;
  }

  nfs_bookmark_fetcher_.reset(new nfs_sync::NfsBookmarkFetcher(getter_.get()));
  if (!nfs_bookmark_fetcher_ || !nfs_bookmark_fetcher_.get()) {
    return false;
  }

  if (logged_in_) {
    LoadFromPrefs();

    ReportLoginSuccess(account_info_.name,
                       account_info_.email,
                       account_info_.id);

    bookmark_model_->AddObserver(this);

    nfs_theme_fetcher_->GetThemeFromServer(
        account_info_.id,
        base::Bind(&NfsSyncService::OnGetThemeRequest,
                   base::Unretained(this)));
    nfs_bookmark_fetcher_->GetBookmarkFromServer(
        account_info_.id,
        base::Bind(&NfsSyncService::OnGetBookmarkData,
                   base::Unretained(this)));
    nfs_password_fetecher_->GetPasswordFromServer(
        account_info_.id,
        base::Bind(&NfsSyncService::OnGetPasswordRequest,
                   base::Unretained(this)));
  }

  return true;
}

void NfsSyncService::AddObserver(nfs_sync::NfsSyncObserver* observer) {
  observers_.AddObserver(observer);
}

void NfsSyncService::RemoveObserver(nfs_sync::NfsSyncObserver* observer) {
  observers_.RemoveObserver(observer);
}

void NfsSyncService::ReportLoginSuccess(const std::string& name,
                                         const std::string& email,
                                         const std::string& id) {
  FOR_EACH_OBSERVER(nfs_sync::NfsSyncObserver, observers_,
                    OnLoginSuccess(name, email, id));
}

void NfsSyncService::ReportLogoutSuccess() {
  if (!logged_in_) {
    return;
  }

  logged_in_ = false;
  bookmark_is_motify_ = false;
  account_info_ = nfs_sync::AccountInfo();

  FOR_EACH_OBSERVER(nfs_sync::NfsSyncObserver, observers_,
                    OnLogoutSuccess());

  pref_service_->SetBoolean(nfs_sync::prefs::kIsLogin, false);

  // 重置本地同步时间以及删除本地帐号信息
  ClearToPrefs();

  bookmark_model_->RemoveObserver(this);

  // 退出之后，删除原来帐号的收藏夹
  const bookmarks::BookmarkNode* bb_node = bookmark_model_->bookmark_bar_node();
  if (!bb_node) {
    return;
  }
  while (bb_node->child_count() > 0) {
    bookmark_model_->Remove(bb_node->GetChild(0));
  }

  //删除账户密码
  std::string url, name, password_decrypt;
  content::BrowserThread::PostTask(
        content::BrowserThread::DB, FROM_HERE,
        base::Bind(&NfsSyncService::ResetLoginDB, base::Unretained(this)));
}

void NfsSyncService::SetAccountInfo(nfs_sync::AccountInfo info) {
  account_info_ = info;
}

void NfsSyncService::Login(const std::string& name,
                            const std::string& email,
                            const std::string& user_id) {
  if (logged_in_) {
    return;
  }

  logged_in_ = true;
  account_info_.name = name;
  account_info_.email = email;
  account_info_.id = user_id;

  SaveToPrefs(name, email, user_id);

  pref_service_->SetBoolean(nfs_sync::prefs::kIsLogin, true);

  // 通过登录页面登录之后，需要判断该帐号是否已经存在
  account_checker_->CheckAccount(
      account_info_,
      base::Bind(&NfsSyncService::OnCheckAccount,
                 base::Unretained(this)));

  bookmark_model_->AddObserver(this);

  ReportLoginSuccess(name, email, user_id);
}

void NfsSyncService::PostThemeToServer(const std::string& theme_id) {
  nfs_theme_fetcher_->PostThemeToServer(
      account_info_.id, theme_id,
      base::Bind(&NfsSyncService::OnGetThemeRequest,
                 base::Unretained(this)));
}

void NfsSyncService::PostPasswordToServer(const std::string& password) {
  nfs_password_fetecher_->PostPasswordToServer(
      password,
      base::Bind(&NfsSyncService::OnPostPasswordRequest,
                 base::Unretained(this)));
}

void NfsSyncService::SyncBookmarkNow() {
  DCHECK(nfs_bookmark_fetcher_ &&  nfs_bookmark_fetcher_.get());
  if (!nfs_bookmark_fetcher_ || !nfs_bookmark_fetcher_.get()) {
    return;
  }

  nfs_bookmark_fetcher_->GetBookmarkFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));
}

void NfsSyncService::SyncThemeNow() {
  DCHECK(nfs_theme_fetcher_ && nfs_theme_fetcher_.get());
  if (!nfs_theme_fetcher_ || !nfs_theme_fetcher_.get()) {
    return;
  }

  nfs_theme_fetcher_->GetThemeFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetThemeRequest,
                 base::Unretained(this)));
}

void NfsSyncService::SyncPasswordNow() {
  DCHECK(nfs_password_fetecher_ && nfs_password_fetecher_.get());
  if (!nfs_password_fetecher_ || !nfs_password_fetecher_.get()) {
    return;
  }

  nfs_password_fetecher_->GetPasswordFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetPasswordRequest,
                 base::Unretained(this)));
}

void NfsSyncService::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(source);
  DCHECK_EQ(source, fetcher_.get());

  if (!fetcher_ || !fetcher_.get()) {
    return;
  }

  if (fetcher_->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      fetcher_->GetResponseCode() != 200) {
    return;
  }
  std::string response_body;
  if (!fetcher_->GetResponseAsString(&response_body)) {
    return;
  }

  std::unique_ptr<base::DictionaryValue> value =
      nfs_sync::JSONStringToDictionary(response_body);
  if (!value || !value.get()) {
    return;
  }

  if (value->HasKey("error")) {
    return;
  }

  DictionaryPrefUpdate update(pref_service_,
                              nfs_sync::prefs::kMotifyTime);
  base::DictionaryValue* dict = update.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }

  base::DictionaryValue* motify_time;
  if (!value->GetDictionary("motifyTime", &motify_time)) {
    return;
  }

  for (base::DictionaryValue::Iterator it(*motify_time);
       !it.IsAtEnd(); it.Advance()) {
    std::string time;
    if (!it.value().GetAsString(&time)) {
      continue;
    }
    dict->SetString(it.key(), time);
  }
}

void NfsSyncService::BookmarkModelLoaded(
    bookmarks::BookmarkModel* model, bool ids_reassigned) {
}

void NfsSyncService::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
}

void NfsSyncService::BookmarkNodeMoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* old_parent,
    int old_index,
    const bookmarks::BookmarkNode* new_parent,
    int new_index) {
  DCHECK(bookmark_model_);

  bookmark_is_motify_ = true;

  nfs_bookmark_fetcher_->GetBookmarkFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));
}

void NfsSyncService::BookmarkNodeAdded(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int index) {
  DCHECK(bookmark_model_);

  bookmark_is_motify_ = true;

  nfs_bookmark_fetcher_->GetBookmarkFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));
}

void NfsSyncService::OnWillRemoveBookmarks(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int old_index,
    const bookmarks::BookmarkNode* node) {}

void NfsSyncService::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked) {
  DCHECK(bookmark_model_);

  bookmark_is_motify_ = true;

  nfs_bookmark_fetcher_->GetBookmarkFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));
}

void NfsSyncService::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  DCHECK(bookmark_model_);

  bookmark_is_motify_ = true;

  nfs_bookmark_fetcher_->GetBookmarkFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));
}

void NfsSyncService::BookmarkNodeChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  DCHECK(bookmark_model_);

  bookmark_is_motify_ = true;

  nfs_bookmark_fetcher_->GetBookmarkFromServer(
      account_info_.id,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));
}

void NfsSyncService::BookmarkMetaInfoChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {}

void NfsSyncService::BookmarkNodeFaviconChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {}

void NfsSyncService::BookmarkNodeChildrenReordered(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {}

void NfsSyncService::LoadFromPrefs() {
  const base::DictionaryValue* value =
      pref_service_->GetDictionary(nfs_sync::prefs::kNfsAccountInfo);
  if (!value) {
    return;
  }

  if (value->empty()) {
    return;
  }

  std::string name, email, id;
  if (!value->GetString(nfs_sync::prefs::kAccountID, &id) ||
      !value->GetString(nfs_sync::prefs::kAccountName, &name) ||
      !value->GetString(nfs_sync::prefs::kAccountEmail, &email)) {
    return;
  }

  account_info_.id = id;
  account_info_.name = name;
  account_info_.email = email;
}

void NfsSyncService::SaveToPrefs(const std::string& name,
                                  const std::string& email,
                                  const std::string& user_id) {
  DictionaryPrefUpdate update(pref_service_,
                              nfs_sync::prefs::kNfsAccountInfo);
  base::DictionaryValue* dict = update.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }

  update->SetString(nfs_sync::prefs::kAccountID, user_id);
  update->SetString(nfs_sync::prefs::kAccountName, name);
  update->SetString(nfs_sync::prefs::kAccountEmail, email);
}

void NfsSyncService::ClearToPrefs() {
  base::DictionaryValue* dict = NULL;

  // 删除本地帐号信息
  DictionaryPrefUpdate account(pref_service_,
                               nfs_sync::prefs::kNfsAccountInfo);
  dict = account.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }
  account->Clear();

  // 本地修改时间重置为0
  DictionaryPrefUpdate motify_time(pref_service_,
                                   nfs_sync::prefs::kMotifyTime);
  dict = motify_time.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }
  for (base::DictionaryValue::Iterator it(*dict);
       !it.IsAtEnd(); it.Advance()) {
    dict->SetString(it.key(), "0");
  }
}

void NfsSyncService::PostAllDataToServer() {
  base::DictionaryValue value;
  value.SetString("userId", account_info_.id);
  value.SetString("accountInfo.id", account_info_.id);
  value.SetString("accountInfo.name", account_info_.name);
  value.SetString("accountInfo.email", account_info_.email);
  value.SetString("syncData.theme", pref_service_->GetString(kCurrentThemeID));

  std::string bookmark = BookmarkToString(bookmark_model_);
  if (bookmark.empty()) {
    return;
  }
  std::string encrypted_bookmark;
  if (!nfs_sync::EncryptAndBase64(bookmark, &encrypted_bookmark)) {
    return;
  }
  value.SetString("syncData.bookmark", encrypted_bookmark);

  std::string data;
  if (!nfs_sync::DictionaryToString(&value, &data)) {
    return;
  }

  std::string url = base::StringPrintf("%s%s%s",
                                       kURL,
                                       "?user_id=",
                                       account_info_.id.c_str());
  fetcher_ =
      net::URLFetcher::Create(GURL(url), net::URLFetcher::POST, this);
  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                                   net::LOAD_BYPASS_CACHE);
  fetcher_->SetExtraRequestHeaders(kContentHeadersJSON);
  fetcher_->SetUploadData(kContentTypeJSON, data);

  fetcher_->Start();
}

void NfsSyncService::ApplyBookmarkData(const bookmarks::BookmarkNode* node,
                                        base::ListValue* children) {
  if (NULL == node) {
    return;
  }

  DCHECK(bookmark_model_);
  if (!bookmark_model_) {
    return;
  }

  const int count = node->child_count();
  for (size_t i = 0; i < children->GetSize(); ++i) {
    base::DictionaryValue* value;
    if (!children->GetDictionary(i, &value)) {
      return;
    }
    if (!value) {
      return;
    }

    std::string type;
    base::string16 name;
    if (!value->HasKey("type") ||
        !value->GetString("type", &type) ||
        !value->HasKey("name") ||
        !value->GetString("name", &name)) {
      return;
    }

    if ("folder" == type && value->HasKey("children")) {
      base::ListValue* new_children;
      if (!value->GetList("children", &new_children)) {
        return;
      }

      const bookmarks::BookmarkNode* new_node = NULL;
      int j;
      for (j = 0; j < node->child_count(); ++j) {
        const bookmarks::BookmarkNode* child_node = node->GetChild(j);
        if (child_node->is_folder() && child_node->GetTitle() == name) {
          new_node = child_node;
          break;
        }
      }
      if (node->child_count() == j) {
        new_node = bookmark_model_->AddFolder(node, node->child_count(), name);
      }

      ApplyBookmarkData(new_node, new_children);
    } else {
      std::string url;
      if (!value->HasKey("url") || !value->GetString("url", &url)) {
        return;
      }

      if (!BookmarkExist(count, node, name, url)) {
        bookmark_model_->AddURL(node, node->child_count(), name, GURL(url));
      }
    }
  }
}

void NfsSyncService::MergeBookmarks(base::ListValue* server_data) {
  if (!server_data || server_data->GetSize() == 0) {
    LOG(ERROR) << "should not be 0";
    return;
  }

  bookmark_model_->RemoveObserver(this);

  const bookmarks::BookmarkNode* bb_node = bookmark_model_->bookmark_bar_node();
  if (!bb_node) {
    return;
  }

  ApplyBookmarkData(bb_node, server_data);

  std::string local_data = BookmarkToString(bookmark_model_);
  if (local_data.empty()) {
    return;
  }
  nfs_bookmark_fetcher_->PostBookmarkToServer(
      account_info_.id, local_data,
      base::Bind(&NfsSyncService::OnGetBookmarkData,
                 base::Unretained(this)));

  bookmark_model_->AddObserver(this);
}

void NfsSyncService::OnCheckAccount(const bool result) {
  // 如果不存在，直接将本地数据发送到服务器
  if (!result) {
    PostAllDataToServer();
  } else { // 如果帐号存在，比较服务器数据修改时间和本地数据修改时间，根据时间判断是否同步数据
    DictionaryPrefUpdate update(pref_service_,
                                nfs_sync::prefs::kMotifyTime);
    base::DictionaryValue* dict = update.Get();
    DCHECK(dict);
    if (dict && !dict->HasKey("bookmark")) {
      dict->SetString("bookmark", "0");
    }
    if (dict && !dict->HasKey("theme")) {
      dict->SetString("theme", "0");
    }

    if (pref_service_ &&
        pref_service_->GetBoolean(prefs::kEnableSyncBookmark)) {
      nfs_bookmark_fetcher_->GetBookmarkFromServer(
          account_info_.id,
          base::Bind(&NfsSyncService::OnGetBookmarkData,
                     base::Unretained(this)));
    }
    if (pref_service_ &&
        pref_service_->GetBoolean(prefs::kEnableSyncSkin)) {
      nfs_theme_fetcher_->GetThemeFromServer(
          account_info_.id,
          base::Bind(&NfsSyncService::OnGetThemeRequest,
                     base::Unretained(this)));
    }
    if (pref_service_ &&
        pref_service_->GetBoolean(prefs::kEnableSyncPassword)) {
      nfs_password_fetecher_->GetPasswordFromServer(
          account_info_.id,
          base::Bind(&NfsSyncService::OnGetPasswordRequest,
                     base::Unretained(this)));
    }
  }
}

void NfsSyncService::OnGetThemeRequest(const std::string& response) {
  std::unique_ptr<base::DictionaryValue> value =
      nfs_sync::JSONStringToDictionary(response);
  if (!value || !value.get()) {
    return;
  }

  if (value->HasKey("error")) {
    return;
  }

  std::string type;
  if (!value->GetString("type", &type)) {
    return;
  }

  DictionaryPrefUpdate update(pref_service_,
                              nfs_sync::prefs::kMotifyTime);
  base::DictionaryValue* dict = update.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }

  if ("get" == type) {
    std::string server_motify_time_str, server_theme_id, local_motify_time_str;
    if (!value->GetString("data", &server_theme_id) ||
        !value->GetString("motifyTime", &server_motify_time_str)) {
      return;
    }

    if (dict->empty() ||
        !dict->GetString("theme", &local_motify_time_str)) {
      nfs_theme_handler_.reset(new nfs_sync::NfsThemeHandle(profile_));
      nfs_theme_handler_->ThemeLoad(server_theme_id);
    } else {
      std::string local_theme_id =
          pref_service_->GetString(kCurrentThemeID);
      if (local_theme_id == server_theme_id) {
        return;
      }

      // 如果本地时间比服务器新，就不修改
      if (local_motify_time_str >= server_motify_time_str) {
        return;
      }

      nfs_theme_handler_.reset(new nfs_sync::NfsThemeHandle(profile_));
      nfs_theme_handler_->ThemeLoad(server_theme_id);
    }
  } else if ("post" == type) {
    std::string motify_time;
    if (!value->GetString("motifyTime", &motify_time)) {
      return;
    }
    dict->SetString("theme", motify_time);
  } else {
    NOTREACHED();
  }
}

void NfsSyncService::OnPostPasswordRequest(const std::string& response) {
}

void NfsSyncService::PostAllPasswordsToServer() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::DB);
  password_manager::PasswordStore* password_store =
      PasswordStoreFactory::GetForProfile(profile_, ServiceAccessType::EXPLICIT_ACCESS).get();
  password_store->PostAllPasswordsToServer();
}

void NfsSyncService::OnGetPasswordRequest(const std::string& response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::unique_ptr<base::DictionaryValue> value =
      nfs_sync::JSONStringToDictionary(response);
  if (!value || !value.get()) {
    return;
  }


  if (value->HasKey("error")) {
    return;
  }

  if (pref_service_ &&
      pref_service_->GetBoolean(prefs::kFirstSyncPassword)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::DB, FROM_HERE,
        base::Bind(&NfsSyncService::PostAllPasswordsToServer,
        base::Unretained(this)));
    pref_service_->SetBoolean(prefs::kFirstSyncPassword, false);
    return;
  }


  DictionaryPrefUpdate update(pref_service_,
                              nfs_sync::prefs::kMotifyTime);
  base::DictionaryValue* dict = update.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }


  std::string local_motify_time_str, server_motify_time_str;
  base::ListValue* password_data;
  if (!value->GetList("data", &password_data) ||
      !value->GetString("passwordUpdateTime", &server_motify_time_str)) {
    return;
  }

  for (size_t i = 0 ; i<password_data->GetSize(); ++i ) {
    base::DictionaryValue* dict_password = NULL;
    password_data ->GetDictionary(i, &dict_password);

    std::string url;
    std::string name;
    std::string password;
    std::string password_decrypt;

    dict_password->GetString("url", &url);
    dict_password->GetString("name", &name);
    dict_password->GetString("password", &password);


    if (!dict_password->GetString("url", &url) ||
        !dict_password->GetString("name", &name) ||
        !dict_password->GetString("password", &password)) {
      NOTREACHED();
      continue;
    }

    nfs_sync::Base64DecodeAndDecrypt(password, &password_decrypt);
    content::BrowserThread::PostTask(
        content::BrowserThread::DB, FROM_HERE,
        base::Bind(&NfsSyncService::AddLoginDB,
        base::Unretained(this), url, name, password_decrypt));
  }
  dict->SetString("password", server_motify_time_str);
}

void NfsSyncService::AddLoginDB(const std::string& url,
    const std::string& name, const std::string& password) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::DB);
  if(url.empty())
    return;

  password_manager::PasswordStore* password_store =
      PasswordStoreFactory::GetForProfile(profile_, ServiceAccessType::EXPLICIT_ACCESS).get();
  autofill::PasswordForm* password_form = new autofill::PasswordForm();
  password_form->signon_realm = url;
  password_form->origin = GURL(url);
  password_form->username_value = base::UTF8ToUTF16(name);
  password_form->password_value = base::UTF8ToUTF16(password);

  const autofill::PasswordForm password_form_const = *password_form;
  password_store->AddLogin(password_form_const, true);
}

void NfsSyncService::ResetLoginDB() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::DB);
  password_manager::PasswordStore* password_store =
      PasswordStoreFactory::GetForProfile(profile_, ServiceAccessType::EXPLICIT_ACCESS).get();
  base::Time start = base::Time();
  base::Time end = base::Time();
  password_store->ResetLoginsDB(start, end, false);
}

void NfsSyncService::OnGetBookmarkData(const std::string& response) {
  std::unique_ptr<base::DictionaryValue> result =
      nfs_sync::JSONStringToDictionary(response);
  if (!result || !result.get()) {
    return;
  }

  if (result->HasKey("error")) {
    LOG(ERROR) << "ERROR";
    return;
  }

  std::string type, motify_time;
  if (!result->GetString("type", &type) ||
      !result->GetString("motifyTime", &motify_time)) {
    return;
  }

  DictionaryPrefUpdate update(pref_service_,
                              nfs_sync::prefs::kMotifyTime);
  base::DictionaryValue* dict = update.Get();
  DCHECK(dict);
  if (!dict) {
    return;
  }

  if ("get" == type) {
    std::string local_motify_time;
    if (!dict->GetString("bookmark", &local_motify_time)) {
      return;
    }

    uint64_t local_time, server_time;
    if (!base::StringToUint64(local_motify_time, &local_time) ||
        !base::StringToUint64(motify_time, &server_time)) {
      return;
    }

    if (local_time < server_time) {
      std::string bookmark_data;
      if (!result->GetString("data", &bookmark_data)) {
        return;
      }

      // 解密
      std::string decrypt;
      if (!nfs_sync::Base64DecodeAndDecrypt(bookmark_data, &decrypt)) {
        return;
      }

      std::unique_ptr<base::DictionaryValue> value =
          nfs_sync::JSONStringToDictionary(decrypt);
      if (!value || !value.get()) {
        return;
      }

      base::ListValue* children_list;
      if (!value->GetList("roots.bookmark_bar.children", &children_list)) {
        return;
      }

      MergeBookmarks(children_list);
    } else if (bookmark_is_motify_) {
      std::string data = BookmarkToString(bookmark_model_);
      if (data.empty()) {
        return;
      }

      nfs_bookmark_fetcher_->PostBookmarkToServer(
          account_info_.id, data,
          base::Bind(&NfsSyncService::OnGetBookmarkData,
                     base::Unretained(this)));
    }
  } else if ("post" == type) {
    dict->SetString("bookmark", motify_time);
  } else {
    NOTREACHED();
  }
}

void NfsSyncService::Cleanup() {
  if (!thread_) {
    // We've already cleaned up.
    return;
  }

  base::Thread* thread = thread_;
  thread_ = nullptr;
  delete thread;
}
