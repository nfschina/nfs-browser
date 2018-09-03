// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/account_login/account_login_handler.h"

#include "base/strings/utf_string_conversions.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/nfs_sync/nfs_sync_service.h"
#include "chrome/browser/nfs_sync/nfs_sync_service_factory.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/local_auth.h"
#include "chrome/browser/ui/views/web_content_dialog_view.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/common/signin_pref_names.h"
#include "components/version_info/version_info.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "third_party/curl/src/include/curl/curl.h"
#include "third_party/curl/src/include/curl/easy.h"

namespace {

const char kURL[] = "https://passport.task.ac.cn/cdos/login";
const char kContentTypeJSON[] = "application/json";
const char kContentHeaders[] =
    "Content-Type: application/json; charset=utf-8";

const char kAccountInfoPref[] = "account_info";
const char kAccountKeyPath[] = "account_id";
const char kAccountEmailPath[] = "email";
const char kAccountGaiaPath[] = "gaia";
const char kAccountHostedDomainPath[] = "hd";
const char kAccountFullNamePath[] = "full_name";
const char kAccountGivenNamePath[] = "given_name";
const char kAccountLocalePath[] = "locale";
const char kAccountPictureURLPath[] = "picture_url";
const char kAccountChildAccountStatusPath[] = "is_child_account";

#if defined(OS_POSIX)
const char* kErrorList[] = {
  "登录成功",
  "用户名或者密码错误",
  "未知错误",
  "网络错误",
  "用户名密码获取失败",
  "URLFetcher构造失败",
  "传递参数构造失败",
  "返回结果获取失败",
  "返回数据获取失败",
  "返回数据解析失败",
  "这台设备已绑定其它帐号(暂不允许多个帐号登录)"
};
#elif defined(OS_WIN)
const wchar_t* kErrorList[] = {
  L"登录成功",
  L"用户名或者密码错误",
  L"未知错误",
  L"网络错误",
  L"用户名密码获取失败",
  L"URLFetcher构造失败",
  L"传递参数构造失败",
  L"返回结果获取失败",
  L"返回数据获取失败",
  L"返回数据解析失败",
  L"这台设备已绑定其它帐号(暂不允许多个帐号登录)"
};
#endif

std::string GetUserAgent() {
  return version_info::GetProductNameAndVersionForUserAgent();
}

}

using content::BrowserThread;

AccountLoginHandler::AccountLoginHandler(content::WebUI* web_ui)
    : profile_(Profile::FromWebUI(web_ui))
    , nfs_sync_service_(nullptr)
    , weak_ptr_factory_(this) {
  if (profile_) {
    nfs_sync_service_ =
        NfsSyncServiceFactory::GetForBrowserContext(profile_);
  }
}

AccountLoginHandler::~AccountLoginHandler() {
}

void AccountLoginHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "isLoggedIn", base::Bind(&AccountLoginHandler::CheckIsLoggedIn,
                               base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "login", base::Bind(&AccountLoginHandler::HandleLogin,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "logout", base::Bind(&AccountLoginHandler::HandleLogout,
                           base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getEmail", base::Bind(&AccountLoginHandler::HandleGetEmail,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getOptions", base::Bind(&AccountLoginHandler::HandleGetOptions,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "changeSyncBookmark", base::Bind(&AccountLoginHandler::HandleChangeSyncBookmark,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "changeSyncSkin", base::Bind(&AccountLoginHandler::HandleChangeSyncSkin,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "changeSyncPassword", base::Bind(&AccountLoginHandler::HandleChangeSyncPassword,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "syncNow", base::Bind(&AccountLoginHandler::HandleSyncNow,
                            base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getAccountId", base::Bind(&AccountLoginHandler::HandleGetAccountId,
                                 base::Unretained(this)));
}

void AccountLoginHandler::CheckIsLoggedIn(const base::ListValue* args) {
  DCHECK(nfs_sync_service_);
  if (!nfs_sync_service_ || !nfs_sync_service_->IsLoggedIn()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "account.isLoggedIn",
        base::FundamentalValue(false));
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
        "account.isLoggedIn",
        base::FundamentalValue(true));
}

void AccountLoginHandler::HandleGetAccountId(const base::ListValue* args) {
  if (!nfs_sync_service_) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "account.getAccountId", base::StringValue(std::string()));
    return;
  }

  nfs_sync::AccountInfo account_info = nfs_sync_service_->GetAccountInfo();
  web_ui()->CallJavascriptFunctionUnsafe(
      "account.getAccountId", base::StringValue(account_info.id));
}

void AccountLoginHandler::HandleLogin(const base::ListValue* args) {
  // 1. 服务构造失败
  // 2. 已登录
  if (!nfs_sync_service_ || nfs_sync_service_->IsLoggedIn()) {
    return;
  }

  std::string email, password;
  if (!args->GetString(0, &email) || !args->GetString(1, &password)) {
    ReportLoginResult(3);
    return;
  }

  fetcher_ = net::URLFetcher::Create(GURL(kURL), net::URLFetcher::POST, this);
  if (!fetcher_ || !fetcher_.get() || !profile_) {
    ReportLoginResult(2);
    return;
  }

  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(profile_->GetRequestContext());
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                         net::LOAD_DO_NOT_SAVE_COOKIES |
                         net::LOAD_IGNORE_ALL_CERT_ERRORS |
                         net::LOAD_DISABLE_CERT_REVOCATION_CHECKING);

  std::string data;
  base::DictionaryValue dict;
  dict.SetString("email", email);
  dict.SetString("password", password);
  if (!base::JSONWriter::Write(dict, &data)) {
    ReportLoginResult(5);
    return;
  }

  fetcher_->SetUploadData(kContentTypeJSON, data);
  fetcher_->SetExtraRequestHeaders(kContentHeaders);

  fetcher_->Start();
}

void AccountLoginHandler::HandleLogout(const base::ListValue* args) {
  if (!nfs_sync_service_) {
    return;
  }

  profile_->GetPrefs()->SetString(
      prefs::kGoogleServicesAccountId, GetAccountID());
  profile_->GetPrefs()->SetString(
      prefs::kGoogleServicesLastUsername, GetAccountEmail());
  profile_->GetPrefs()->SetString(
      prefs::kGoogleServicesLastAccountId, GetAccountID());

  nfs_sync_service_->ReportLogoutSuccess();

  nfsbrowser::WebContentDialogView::Hide(web_ui()->GetWebContents());
}

void AccountLoginHandler::HandleGetEmail(const base::ListValue* args) {
  web_ui()->CallJavascriptFunctionUnsafe("account.showEmail",
    base::StringValue(GetAccountEmail()));
}

void AccountLoginHandler::HandleGetOptions(const base::ListValue* args) {
  bool is_sync_bookmark =
      profile_->GetPrefs()->GetBoolean(prefs::kEnableSyncBookmark);
  bool is_sync_skin =
      profile_->GetPrefs()->GetBoolean(prefs::kEnableSyncSkin);
  bool is_sync_password =
      profile_->GetPrefs()->GetBoolean(prefs::kEnableSyncPassword);

  web_ui()->CallJavascriptFunctionUnsafe("account.showOptions",
    base::FundamentalValue(is_sync_bookmark),
    base::FundamentalValue(is_sync_skin),
    base::FundamentalValue(is_sync_password));
}

void AccountLoginHandler::HandleChangeSyncBookmark(const base::ListValue* args) {
  bool is_sync_bookmark = true;
  if (!args->GetBoolean(0, &is_sync_bookmark)) {
    return ;
  }

  profile_->GetPrefs()->SetBoolean(prefs::kEnableSyncBookmark, is_sync_bookmark);
}

void AccountLoginHandler::HandleChangeSyncSkin(const base::ListValue* args) {
  bool is_sync_skin;
  if (!args->GetBoolean(0, &is_sync_skin)) {
    return ;
  }

  profile_->GetPrefs()->SetBoolean(prefs::kEnableSyncSkin, is_sync_skin);
}

void AccountLoginHandler::HandleChangeSyncPassword(const base::ListValue* args) {
  bool is_sync_password;
  if (!args->GetBoolean(0, &is_sync_password)) {
    return ;
  }

  profile_->GetPrefs()->SetBoolean(prefs::kEnableSyncPassword, is_sync_password);
}

void AccountLoginHandler::HandleSyncNow(const base::ListValue* args) {
  DCHECK(profile_ && nfs_sync_service_);
  if (!profile_ || !nfs_sync_service_) {
    return;
  }

  bool is_sync_skin = profile_->GetPrefs()->GetBoolean(prefs::kEnableSyncSkin);
  if (is_sync_skin) {
    nfs_sync_service_->SyncThemeNow();
  }

  bool is_sync_bookmark =
      profile_->GetPrefs()->GetBoolean(prefs::kEnableSyncBookmark);
  if (is_sync_bookmark) {
    nfs_sync_service_->SyncBookmarkNow();
  }

  bool is_sync_password =
      profile_->GetPrefs()->GetBoolean(prefs::kEnableSyncPassword);
  if (is_sync_password) {
    nfs_sync_service_->SyncPasswordNow();
  }
}

void AccountLoginHandler::OnURLFetchComplete(const net::URLFetcher* source) {
  if (!source) {
    ReportLoginResult(4);
    return;
  }

  if (source->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      source->GetResponseCode() != net::HTTP_OK) {
    ReportLoginResult(5);
    return;
  }

  std::string response_body;
  if (!source->GetResponseAsString(&response_body)) {
    ReportLoginResult(6);
    return;
  }

  auto dict = base::DictionaryValue::From(base::JSONReader::Read(response_body));
  if (!dict) {
    ReportLoginResult(7);
    return;
  }

  bool status;
  if (!dict->GetBoolean("status", &status)) {
    ReportLoginResult(7);
    return;
  }

  if (!status) {
    std::string error_message;
    if (!dict->GetString("error", &error_message)) {
      ReportLoginResult(7);
      return;
    }

    ReportLoginResult(1);
    return;
  }

  std::string username, email, user_id;
  if (!dict->GetString("userinfo.username", &username) ||
      !dict->GetString("userinfo.email", &email) ||
      !dict->GetString("userinfo.id", &user_id)) {
    ReportLoginResult(7);
  }

  if (nfs_sync_service_) {
    nfs_sync_service_->ReportLoginSuccess(username, email, user_id);
  }

  ReportLoginResult(0);
}

void AccountLoginHandler::ReportLoginResult(int error_code) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "account.loginResult",
      base::FundamentalValue(error_code),
      base::StringValue(kErrorList[error_code]));
}

std::string AccountLoginHandler::GetAccountID() {
  if (!profile_->GetPrefs()) {
    return "";
  }

  const base::ListValue* list =
      profile_->GetPrefs()->GetList(kAccountInfoPref);
  for (size_t i = 0; i < list->GetSize(); ++i) {
    const base::DictionaryValue* dict;
    if (list->GetDictionary(i, &dict)) {
      std::string account_id;
      if (dict->GetString(kAccountKeyPath, &account_id)) {
        return account_id;
      }
    }
  }

  return "";
}

std::string AccountLoginHandler::GetAccountEmail() {
  if (!nfs_sync_service_) {
    return std::string();
  }

  nfs_sync::AccountInfo account_info = nfs_sync_service_->GetAccountInfo();
  return account_info.name;
}


void AccountLoginHandler::SaveToPref(AccountInfo info) {
  if (!profile_->GetPrefs()) {
    return;
  }

  base::DictionaryValue* dict = nullptr;
  base::string16 account_id_16 = base::UTF8ToUTF16(info.account_id);
  ListPrefUpdate update(profile_->GetPrefs(), kAccountInfoPref);
  for (size_t i = 0; i < update->GetSize(); ++i, dict = nullptr) {
    if (update->GetDictionary(i, &dict)) {
      base::string16 value;
      if (dict->GetString(kAccountKeyPath, &value) && value == account_id_16)
        break;
    }
  }

  if (!dict) {
    dict = new base::DictionaryValue();
    update->Append(base::WrapUnique(dict));
    dict->SetString(kAccountKeyPath, account_id_16);
  }

  dict->SetString(kAccountEmailPath, info.email);
  dict->SetString(kAccountGaiaPath, info.account_id);
  dict->SetString(kAccountHostedDomainPath, info.hosted_domain);
  dict->SetString(kAccountFullNamePath, info.full_name);
  dict->SetString(kAccountGivenNamePath, info.given_name);
  dict->SetString(kAccountLocalePath, info.locale);
  dict->SetString(kAccountPictureURLPath, info.picture_url);
  dict->SetBoolean(kAccountChildAccountStatusPath, info.is_child_account);
}

///////////////////////////////////////////////////////////////////////////////
//
// MyURLRequestContextGetter
//
///////////////////////////////////////////////////////////////////////////////

MyURLRequestContextGetter::MyURLRequestContextGetter(
    IOThread* io_thread)
    : network_task_runner_(
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)) {}

MyURLRequestContextGetter::~MyURLRequestContextGetter() {}

net::URLRequestContext* MyURLRequestContextGetter::GetURLRequestContext() {
  if (!url_request_context_) {
    // Use default values
    net::URLRequestContextBuilder builder;

    builder.set_proxy_service(net::ProxyService::CreateDirect());
    builder.set_user_agent(GetUserAgent());
    url_request_context_ = builder.Build();
  }
  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
MyURLRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}
