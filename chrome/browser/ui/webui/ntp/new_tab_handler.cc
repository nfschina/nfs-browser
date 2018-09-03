#include "chrome/browser/ui/webui/ntp/new_tab_handler.h"

#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_constants.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_helpers.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/search_engines/ui_thread_search_terms_data.h"
#include "chrome/browser/thumbnails/thumbnail_service.h"
#include "chrome/browser/thumbnails/thumbnail_service_factory.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/history/core/browser/top_sites.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

#if defined(OS_POSIX)
#define STRING_LITERAL(x) x
typedef char char16;
#elif defined(OS_WIN)
#define STRING_LITERAL(x) L ## x
typedef wchar_t char16;
#endif

using namespace extensions;

using FetchThumbnailResult = ThumbnailDialog::FetchThumbnailResult;

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::ManagedBookmarkService;
using content::BrowserThread;
using content::WebContents;
using content::WebUI;
using extensions::api::bookmarks::BookmarkTreeNode;
using history::TopSitesObserver;

namespace {

#define default_search_icon "nfsbrowser://resources/images/search/search.png"
#define default_thumbnail "images/none.png"

const struct PrepopulatedWebsite {
  const char* url;
  const char16* title;
} kWebsiteList[] = {
  {"https://www.baidu.com/", STRING_LITERAL("百度")},
  {"http://www.iqiyi.com/", STRING_LITERAL("爱奇艺")},
  {"https://www.jd.com/", STRING_LITERAL("京东")},
  {"https://www.taobao.com/", STRING_LITERAL("淘宝")},
  {"http://www.weibo.com/", STRING_LITERAL("新浪微博")},
  {"http://www.sina.com.cn/", STRING_LITERAL("新浪")},
  {"http://www.youku.com/", STRING_LITERAL("优酷")},
};

const struct PrepopulatedSearchEnginesIcon {
  const char* icon;
} kPrepopulatedSearchEnginesIcon[] = {
  {"nfsbrowser://resources/images/search/search-baidu.png"},
  {"nfsbrowser://resources/images/search/search-bing.png"},
  {"nfsbrowser://resources/images/search/search-google.png"},
  {"nfsbrowser://resources/images/search/search-sogou.png"},
};

const struct RecommendedWebsite {
  const char* url;
  const char16* title;
  const char* thumbnail;
} kRecommendedList[] = {
  {"https://www.baidu.com/", STRING_LITERAL("百度")},
  {"http://www.iqiyi.com/", STRING_LITERAL("爱奇艺")},
  {"https://www.jd.com/", STRING_LITERAL("京东")},
  {"https://www.taobao.com/", STRING_LITERAL("淘宝")},
  {"http://www.qzone.com/", STRING_LITERAL("QQ空间")},
  {"http://www.weibo.com/", STRING_LITERAL("新浪微博")},
  {"http://www.sina.com.cn/", STRING_LITERAL("新浪")},
  {"http://www.youku.com/", STRING_LITERAL("优酷")},
};

Browser* CreateIncognitoBrowser(Profile* profile) {
  Browser* incognito = new Browser(
      Browser::CreateParams(profile->GetOffTheRecordProfile()));

  return incognito;
}

bool GetBookmarkIdAsInt64(const std::string& id_string, int64_t* id) {
  if(base::StringToInt64(id_string, id))
    return true;
  return false;
}

const BookmarkNode* GetBookmarkNodeFromId(Profile* profile, const std::string& id_string) {
  int64_t id;
  if(!GetBookmarkIdAsInt64(id_string, &id))
    return NULL;

  const BookmarkNode* node = bookmarks::GetBookmarkNodeByID(
      BookmarkModelFactory::GetForBrowserContext(profile), id);
  return node;
}

}  // namespace

NewTabHandler::NewTabHandler(Profile* profile)
    : profile_(profile)
    , thumbnail_service_(ThumbnailServiceFactory::GetForProfile(profile))
    , top_sites_(TopSitesFactory::GetForProfile(profile))
    , template_url_service_(TemplateURLServiceFactory::GetForProfile(profile))
    , weak_ptr_factory_(this) {
  DCHECK(template_url_service_);
  DCHECK(profile_);

  if (top_sites_) {
    top_sites_->AddObserver(this);
  }

  if (template_url_service_) {
    template_url_service_->AddObserver(this);
    const TemplateURL* default_search_provider =
        template_url_service_->GetDefaultSearchProvider();
    if (default_search_provider) {
      previous_default_search_provider_.reset(
          new TemplateURLData(default_search_provider->data()));
    }
  }
}

NewTabHandler::~NewTabHandler() {
  if (template_url_service_) {
    template_url_service_->RemoveObserver(this);
  }

  if (top_sites_) {
    top_sites_->RemoveObserver(this);
  }
}

// WebUIMessageHandler implementation.
void NewTabHandler::RegisterMessages() {
  // 注册获取书签的回调函数
  web_ui()->RegisterMessageCallback(
    "getBookmarkNodeByID",
    base::Bind(&NewTabHandler::HandleGetBookmarkNodeByID,
      base::Unretained(this)));

  // 注册获取当前搜索引擎的回调函数
  web_ui()->RegisterMessageCallback(
    "getDefaultSearchEngine",
    base::Bind(&NewTabHandler::HandleGetDefaultSearchEngine,
               base::Unretained(this)));

  // 注册获取默认网站列表的函数
  web_ui()->RegisterMessageCallback(
    "getDefaultWebsites",
    base::Bind(&NewTabHandler::HandleGetDefaultWebsites,
               base::Unretained(this)));

  // 注册获取推荐网站列表的函数
  web_ui()->RegisterMessageCallback(
    "getRecommendedWebsites",
    base::Bind(&NewTabHandler::HandleGetRecommendedWebsites,
               base::Unretained(this)));

  // 注册获取搜索引擎的回调函数
  web_ui()->RegisterMessageCallback(
    "getSearchEngines",
    base::Bind(&NewTabHandler::HandleGetSearchEngines,
               base::Unretained(this)));

  // 注册获取缩略图的回调函数
  web_ui()->RegisterMessageCallback(
    "getThumbnail",
    base::Bind(&NewTabHandler::HandleGetThumbnail,
      base::Unretained(this)));

  // 注册获取最近常用网址的回调函数
  web_ui()->RegisterMessageCallback(
    "getTopSites",
    base::Bind(&NewTabHandler::HandleGetTopSites,
      base::Unretained(this)));

  // 注册打开网页的回调函数
  web_ui()->RegisterMessageCallback(
    "openURL",
    base::Bind(&NewTabHandler::HandleOpenURL,
               base::Unretained(this)));

  // 注册设置搜索引擎的回调函数
  web_ui()->RegisterMessageCallback(
    "setSearchEngine",
    base::Bind(&NewTabHandler::HandleSetSearchEngine,
               base::Unretained(this)));

  // 注冊搜索的回调函數
  web_ui()->RegisterMessageCallback(
    "startSearch",
    base::Bind(&NewTabHandler::HandleStartSearch,
               base::Unretained(this)));

  // 判断TopSites数据库是否加载完成
  web_ui()->RegisterMessageCallback(
    "isDBLoaded",
    base::Bind(&NewTabHandler::HandleDBIsLoaded,
               base::Unretained(this)));
}

void NewTabHandler::OnTemplateURLServiceChanged() {
  const TemplateURL* template_url =
      template_url_service_->GetDefaultSearchProvider();
  bool default_search_provider_changed = !TemplateURL::MatchesData(
      template_url, previous_default_search_provider_.get(),
      UIThreadSearchTermsData(profile_));
  if (default_search_provider_changed) {
    previous_default_search_provider_.reset(
        template_url ? new TemplateURLData(template_url->data()) : NULL);

    base::DictionaryValue defaultSearchEngineValue;
    defaultSearchEngineValue.SetString("name", template_url->short_name());
    defaultSearchEngineValue.SetString("url", template_url->url());

    std::string favicon_string = template_url->favicon_url().spec();
    for (size_t i = 0; i < arraysize(kPrepopulatedSearchEnginesIcon); ++i) {
      if (kPrepopulatedSearchEnginesIcon[i].icon == favicon_string) {
        defaultSearchEngineValue.SetString("favicon", favicon_string);
        break;
      } else {
        defaultSearchEngineValue.SetString("favicon", default_search_icon);
      }
    }

    if (profile_->IsOffTheRecord())
      web_ui()->CallJavascriptFunctionUnsafe("incognito.onDefaultSearchEngineChanged", defaultSearchEngineValue);
    else
      web_ui()->CallJavascriptFunctionUnsafe("ntp.onSearchEngineChanged", defaultSearchEngineValue);
  }
}

// TopSitesObserver implemention.

void NewTabHandler::TopSitesLoaded(history::TopSites* top_sites) {
}

void NewTabHandler::TopSitesChanged(history::TopSites* top_sites,
                                    TopSitesObserver::ChangeReason change_reason) {

}

void NewTabHandler::HandleGetBookmarkNodeByID(const base::ListValue* args) {
  std::string id;
  std::unique_ptr<base::ListValue> bookmarks_list;

  if (args->empty()) {
    id = "1";
  }
  else {
    if (!args->GetString(0, &id)) {
      return ;
    }
  }

  const BookmarkNode* node = GetBookmarkNodeFromId(profile_, id);
  if (!node)
    return ;

  std::vector<BookmarkTreeNode> nodes;
  int child_count = node->child_count();
  for (int i = 0; i < child_count; ++i) {
    const BookmarkNode* child = node->GetChild(i);
    bookmark_api_helpers::AddNode(
      ManagedBookmarkServiceFactory::GetForProfile(profile_),
      child, &nodes, false);
  }
  bookmarks_list =api::bookmarks::GetChildren::Results::Create(nodes);

  base::ListValue* list = NULL;
  if (!bookmarks_list->GetList(0, &list))
    return ;

  DCHECK(list);
  if (!list)
    return ;

  base::ListValue listValue;
  for (size_t i = 0; i < list->GetSize(); ++i) {
    std::string tmpId, tmpUrl, tmpTitle;
    std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
    base::DictionaryValue* temp = NULL;


    if (!list->GetDictionary(i, &temp))
      return ;
    DCHECK(temp);
    if (!temp)
      return ;

    if (temp->GetString("id", &tmpId))
      value->SetString("id", tmpId);
    if (temp->GetString("url", &tmpUrl))
      value->SetString("url", tmpUrl);
    else
      value->SetString("url", "");
    if (temp->GetString("title", &tmpTitle))
      value->SetString("title", tmpTitle);

    listValue.Set(i, std::move(value));
  }

  if (args->empty())
    web_ui()->CallJavascriptFunctionUnsafe("ntp.getBookmarkNodeByID", base::StringValue(""), listValue);
  else
    web_ui()->CallJavascriptFunctionUnsafe("ntp.getBookmarkNodeByID", base::StringValue(id), listValue);
}

void NewTabHandler::HandleGetDefaultSearchEngine(const base::ListValue* args) {
  base::DictionaryValue defaultSearchEngineValue;
  const TemplateURL* defaultSearchEngine =
      template_url_service_->GetDefaultSearchProvider();

  defaultSearchEngineValue.SetString("name", defaultSearchEngine->short_name());
  defaultSearchEngineValue.SetString("url", defaultSearchEngine->url());
  defaultSearchEngineValue.SetString("favicon", defaultSearchEngine->favicon_url().spec());

  std::string favicon_string = defaultSearchEngine->favicon_url().spec();
  for (size_t i = 0; i < arraysize(kPrepopulatedSearchEnginesIcon); ++i) {
    if (kPrepopulatedSearchEnginesIcon[i].icon == favicon_string) {
      defaultSearchEngineValue.SetString("favicon", favicon_string);
      break;
    } else {
      defaultSearchEngineValue.SetString("favicon", default_search_icon);
    }
  }

  if (profile_->IsOffTheRecord())
    web_ui()->CallJavascriptFunctionUnsafe("incognito.getDefaultSearchEngine", defaultSearchEngineValue);
  else
    web_ui()->CallJavascriptFunctionUnsafe("ntp.getDefaultSearchEngine", defaultSearchEngineValue);

  return ;
}

void NewTabHandler::HandleGetDefaultWebsites(const base::ListValue* args) {
  base::ListValue website_list;

  for (size_t i = 0; i < arraysize(kWebsiteList); ++i) {
    std::unique_ptr<base::DictionaryValue> website(new base::DictionaryValue());

    std::string thumbnail =
        std::string("nfsbrowser://capture/") + kWebsiteList[i].url;
    website->SetString("title", kWebsiteList[i].title);
    website->SetString("url", kWebsiteList[i].url);
    website->SetString("thumbnail", thumbnail);
    website->SetInteger("status", 0);

    website_list.Set(i, std::move(website));
  }

  web_ui()->CallJavascriptFunctionUnsafe("ntp.getDefaultWebsites", website_list);
}

void NewTabHandler::HandleGetRecommendedWebsites(const base::ListValue* args) {
  base::ListValue website_list;

  for (size_t i = 0; i < arraysize(kRecommendedList); ++i) {
    std::unique_ptr<base::DictionaryValue> website(new base::DictionaryValue());

    std::string thumbnail =
        std::string("nfsbrowser://capture/") + kRecommendedList[i].url;
    website->SetString("title", kRecommendedList[i].title);
    website->SetString("url", kRecommendedList[i].url);
    website->SetString("thumbnail", thumbnail);
    // website->SetInteger("status", kRecommendedList[i].status);

    website_list.Set(i, std::move(website));
  }

  web_ui()->CallJavascriptFunctionUnsafe("ntp.getRecommendedWebsites", website_list);
}

void NewTabHandler::HandleGetSearchEngines(const base::ListValue* args) {
  if (!template_url_service_)
    return ;

  base::ListValue searchEngineList;
  TemplateURLService::TemplateURLVector searchEngineVector =
      template_url_service_->GetTemplateURLs();

  size_t index = 0;
  for (size_t i = 0; i < searchEngineVector.size(); ++i) {
    if (searchEngineVector[i]->show_in_default_list()) {
      std::unique_ptr<base::DictionaryValue> searchEngineValue(new base::DictionaryValue());

      searchEngineValue->SetString("name", searchEngineVector[i]->short_name());
      searchEngineValue->SetString("url", searchEngineVector[i]->url());
      searchEngineValue->SetInteger("index", i);

      std::string favicon_string = searchEngineVector[i]->favicon_url().spec();
      for (size_t i = 0; i < arraysize(kPrepopulatedSearchEnginesIcon); ++i) {
        if (kPrepopulatedSearchEnginesIcon[i].icon == favicon_string) {
          searchEngineValue->SetString("favicon", favicon_string);
          break;
        } else {
          searchEngineValue->SetString("favicon", default_search_icon);
        }
      }

      searchEngineList.Set(index++, std::move(searchEngineValue));
    }
  }

  if (profile_->IsOffTheRecord())
    web_ui()->CallJavascriptFunctionUnsafe("incognito.getSearchEngines", searchEngineList);
  else
    web_ui()->CallJavascriptFunctionUnsafe("ntp.getSearchEngines", searchEngineList);

  return ;
}

void NewTabHandler::HandleGetThumbnail(const base::ListValue* args) {
  int index = 0;
  std::string url_string;
  if (!args->GetInteger(0, &index) || !args->GetString(1, &url_string)) {
    return ;
  }

  GURL url(url_string);
  base::DictionaryValue value;
  scoped_refptr<base::RefCountedMemory> data;

  if (thumbnail_service_->GetPageThumbnail(url, false, &data)) {
    value.SetInteger("status", 0);
    value.SetInteger("index", index);
    value.SetString("thumbnail", "nfsbrowser://thumb/" + url_string);
    web_ui()->CallJavascriptFunctionUnsafe("ntp.getThumbnail", value);

    return ;
  }

  if (top_sites_->GetPageCapture(url, &data)) {
    value.SetInteger("status", 0);
    value.SetInteger("index", index);
    value.SetString("thumbnail", "nfsbrowser://capture/" + url_string);
    web_ui()->CallJavascriptFunctionUnsafe("ntp.getThumbnail", value);

    return ;
  }

  ThumbnailDialog::FetchThumbnailCallback callback =
      base::Bind(&NewTabHandler::FetchThumbnail,
                 weak_ptr_factory_.GetWeakPtr());

  ThumbnailDialog::CreateWebWidget(profile_, index, url, callback);

  return ;
}

void NewTabHandler::HandleGetTopSites(const base::ListValue* args) {
  if (!top_sites_) {
    return ;
  }

  top_sites_->GetMostVisitedURLs(
      base::Bind(&NewTabHandler::OnMostVisitedURLsAvailable,
                 weak_ptr_factory_.GetWeakPtr()), false);
}

/**
  Open Url
  0: open in new tab
  1: open in new window
  2: open in incognito window
**/
void NewTabHandler::HandleOpenURL(const base::ListValue* args) {
  int type = 0;
  std::string url_string;

  if (!args->GetInteger(0, &type) || !args->GetString(1, &url_string))
    return ;

  GURL url(url_string);
  if (!url.is_valid())
    return ;

  switch(type) {
    case 0: {
      chrome::NavigateParams params(profile_, url, ui::PAGE_TRANSITION_AUTO_BOOKMARK);
      params.disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
      params.source_contents = chrome::FindLastActiveWithProfile(profile_)->tab_strip_model()->GetActiveWebContents();
      chrome::Navigate(&params);

      break;
    }

    case 1:{
      chrome::NavigateParams params(profile_, url, ui::PAGE_TRANSITION_AUTO_BOOKMARK);
      params.disposition = WindowOpenDisposition::NEW_WINDOW;
      params.source_contents = chrome::FindLastActiveWithProfile(profile_)->tab_strip_model()->GetActiveWebContents();
      chrome::Navigate(&params);

      break;
    }

    case 2: {
      Browser* incognito = CreateIncognitoBrowser(profile_);

      chrome::NavigateParams params(incognito, url, ui::PAGE_TRANSITION_AUTO_BOOKMARK);
      params.window_action = chrome::NavigateParams::SHOW_WINDOW;
      chrome::Navigate(&params);

      break;
    }

    default:
      break;
  }
}

void NewTabHandler::HandleSetSearchEngine(const base::ListValue* args) {
  int index = 0;
  if (!args->GetInteger(0, &index)) {
    return ;
  }

  TemplateURLService::TemplateURLVector searchEngineVector =
      template_url_service_->GetTemplateURLs();

  template_url_service_->SetUserSelectedDefaultSearchProvider(searchEngineVector[index]);
}

void NewTabHandler::HandleStartSearch(const base::ListValue* args) {
  std::string query_string;
  if (!args->GetString(0, &query_string))
    return ;

  const TemplateURL* provider_url =
    template_url_service_->GetDefaultSearchProvider();

  const TemplateURLRef& search_url = provider_url->url_ref();
  DCHECK(search_url.SupportsReplacement(template_url_service_->search_terms_data()));
  std::unique_ptr<TemplateURLRef::SearchTermsArgs> search_terms_args(new TemplateURLRef::SearchTermsArgs(base::UTF8ToUTF16(query_string)));
  GURL destination_url = GURL(search_url.ReplaceSearchTerms(*search_terms_args.get(),
      template_url_service_->search_terms_data()));

  chrome::NavigateParams params(profile_, destination_url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
  params.source_contents = chrome::FindLastActiveWithProfile(profile_)->tab_strip_model()->GetActiveWebContents();

  chrome::Navigate(&params);
}

void NewTabHandler::HandleDBIsLoaded(const base::ListValue* args) {
  if (top_sites_) {
    if (top_sites_->loaded()) {
      web_ui()->CallJavascriptFunctionUnsafe("ntp.setThumbnail");
      return ;
    }
  }

  base::ListValue value;
  BrowserThread::PostDelayedTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&NewTabHandler::HandleDBIsLoaded,
                 base::Unretained(this), &value),
      base::TimeDelta::FromMilliseconds(10));
}

void NewTabHandler::OnMostVisitedURLsAvailable(const history::MostVisitedURLList& data) {
  base::ListValue pages_value;
  for(size_t i = 0; i < data.size(); i++) {
    const history::MostVisitedURL& url = data[i];
    if(!url.url.is_empty()) {
      auto page_value = base::MakeUnique<base::DictionaryValue>();
      page_value->SetString("url", url.url.spec());
      page_value->SetString("ico", "nfsbrowser://favicon/" + url.url.spec());

      if(url.title.empty())
        page_value->SetString("title", url.url.spec());
      else
        page_value->SetString("title", url.title);

      pages_value.Append(std::move(page_value));
    }
  }

  web_ui()->CallJavascriptFunctionUnsafe("ntp.getTopSites", pages_value);
}

void NewTabHandler::FetchThumbnail(const FetchThumbnailResult result,
                                   const int index, const GURL& url) {
  base::DictionaryValue value;

  switch(result) {
    case FetchThumbnailResult::FETCH_THUMBNAIL_SUCCESS:
    {
      value.SetInteger("status", 0);
      value.SetInteger("index", index);
      value.SetString("thumbnail", "nfsbrowser://capture/" + url.spec());
      web_ui()->CallJavascriptFunctionUnsafe("ntp.getThumbnail", value);

      return ;
    }

    default :
    {
      value.SetInteger("status", 1);
      value.SetInteger("index", index);
      value.SetString("thumbnail", default_thumbnail);
      web_ui()->CallJavascriptFunctionUnsafe("ntp.getThumbnail", value);

      return ;
    }
  }

}
