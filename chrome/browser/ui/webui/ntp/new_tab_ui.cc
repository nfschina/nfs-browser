// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"

#include <memory>

#include "base/i18n/rtl.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/frame/browser_view_layout.h"
#include "chrome/browser/ui/webui/ntp/new_tab_handler.h"
#include "chrome/browser/ui/webui/ntp/ntp_resource_cache.h"
#include "chrome/browser/ui/webui/ntp/ntp_resource_cache_factory.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/theme_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"
#include "ui/resources/grit/webui_resources_nfs.h"

#if defined(ENABLE_THEMES)
#include "chrome/browser/ui/webui/theme_handler.h"
#endif

using content::BrowserThread;
using content::WebUIController;

namespace {

// Strings sent to the page via jstemplates used to set the direction of the
// HTML document based on locale.
const char kRTLHtmlTextDirection[] = "rtl";
const char kLTRHtmlTextDirection[] = "ltr";

const struct Resource{
  const char* filename;
  int identifier;
  const char* mime_type;
} kResources[] = {
    {"images/none.png", IDR_CDOS_THUMB_DEFAULT, "image/png"},
    {"images/wait.gif", IDR_CDOS_WAIT, "image/gif"},

    {"images/baidu.png", IDR_CDOS_THUMB_BAIDU, "image/png"},
    {"images/iqiyi.png", IDR_CDOS_THUMB_IQIYI, "image/png"},
    {"images/jd.png", IDR_CDOS_THUMB_JD, "image/png"},
    {"images/qqzone.png", IDR_CDOS_THUMB_QQZONE, "image/png"},
    {"images/taobao.png", IDR_CDOS_THUMB_TAOBAO, "image/png"},
    {"images/weibo.png", IDR_CDOS_THUMB_WEIBO, "image/png"},
    {"images/xinlang.png", IDR_CDOS_THUMB_XINLANG, "image/png"},
    {"images/youku.png", IDR_CDOS_THUMB_YOUKU, "image/png"},
    {"images/ntp_favicon.ico", IDR_CDOS_THUMB_NTP_FAVICON, "image/x-icon"},
    // {"images/incognito_tab_favicon.ico", IDR_CDOS_THUMB_INCOGNITO_TAB_FAVICON, "image/x-icon"},

};

const char* GetHtmlTextDirection(const base::string16& text) {
  if (base::i18n::IsRTL() && base::i18n::StringContainsStrongRTLChars(text))
    return kRTLHtmlTextDirection;
  else
    return kLTRHtmlTextDirection;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// NewTabUI

NewTabUI::NewTabUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->OverrideTitle(l10n_util::GetStringUTF16(IDS_NEW_TAB_TITLE));

  Profile* profile = GetProfile();
  web_ui->AddMessageHandler(new NewTabHandler(profile));

  std::unique_ptr<NewTabHTMLSource> html_source(
      new NewTabHTMLSource(profile->GetOriginalProfile()));

  // content::URLDataSource assumes the ownership of the html_source.
  content::URLDataSource::Add(profile, html_source.release());

  registrar_.Add(this, chrome::NOTIFICATION_NTP_BACKGROUND_OFFSET,
      content::Source<BrowserViewLayout>(nullptr));
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
                 content::Source<ThemeService>(
                     ThemeServiceFactory::GetForProfile(GetProfile())));
}

NewTabUI::~NewTabUI() {
  registrar_.RemoveAll();
}

// static
void NewTabUI::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
}

// static
base::RefCountedMemory* NewTabUI::GetFaviconResourceBytes(
      ui::ScaleFactor scale_factor) {
  return ResourceBundle::GetSharedInstance().
      LoadDataResourceBytesForScale(IDR_CDOS_NTP_FAVICON_PNG, scale_factor);
}

// static
bool NewTabUI::IsNewTab(const GURL& url) {
  return url.GetOrigin() == GURL(chrome::kChromeUINewTabURL).GetOrigin();
}

// static
bool NewTabUI::ShouldShowApps() {
  return false;
}

// static
void NewTabUI::SetUrlTitleAndDirection(base::DictionaryValue* dictionary,
                                       const base::string16& title,
                                       const GURL& gurl) {
  dictionary->SetString("url", gurl.spec());

  bool using_url_as_the_title = false;
  base::string16 title_to_set(title);
  if (title_to_set.empty()) {
    using_url_as_the_title = true;
    title_to_set = base::UTF8ToUTF16(gurl.spec());
  }

  // We set the "dir" attribute of the title, so that in RTL locales, a LTR
  // title is rendered left-to-right and truncated from the right. For example,
  // the title of http://msdn.microsoft.com/en-us/default.aspx is "MSDN:
  // Microsoft developer network". In RTL locales, in the [New Tab] page, if
  // the "dir" of this title is not specified, it takes Chrome UI's
  // directionality. So the title will be truncated as "soft developer
  // network". Setting the "dir" attribute as "ltr" renders the truncated title
  // as "MSDN: Microsoft D...". As another example, the title of
  // http://yahoo.com is "Yahoo!". In RTL locales, in the [New Tab] page, the
  // title will be rendered as "!Yahoo" if its "dir" attribute is not set to
  // "ltr".
  std::string direction;
  if (using_url_as_the_title)
    direction = kLTRHtmlTextDirection;
  else
    direction = GetHtmlTextDirection(title);

  dictionary->SetString("title", title_to_set);
  dictionary->SetString("direction", direction);
}

// static
void NewTabUI::SetFullNameAndDirection(const base::string16& full_name,
                                       base::DictionaryValue* dictionary) {
  dictionary->SetString("full_name", full_name);
  dictionary->SetString("full_name_direction", GetHtmlTextDirection(full_name));
}

Profile* NewTabUI::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}

void NewTabUI::Observe(int type,
                 const content::NotificationSource& source,
                 const content::NotificationDetails& details) {
  int offset = 0;
  std::string bgOffset;
  base::DictionaryValue dictionary;
  bool has_custom_bg = false;
  switch (type) {
    case chrome::NOTIFICATION_NTP_BACKGROUND_OFFSET:
      offset = *(content::Details<int>(details).ptr());
      bgOffset = ThemeProperties::OffsetToString(0, -offset);
      web_ui()->CallJavascriptFunctionUnsafe("ntp.setBackgroundOffset", base::StringValue(bgOffset));
      break;

    case chrome::NOTIFICATION_BROWSER_THEME_CHANGED:
      if (GetProfile()->GetProfileType() == Profile::INCOGNITO_PROFILE) {
        base::DictionaryValue dictionary;
        int is_icons_dark = ThemeService::GetThemeProviderForProfile(
            GetProfile()).GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK);
        dictionary.SetString("is_icons_dark", is_icons_dark ? "true" : "false");
        web_ui()->CallJavascriptFunctionUnsafe("incognito.themeChanged", dictionary);
      } else {
        has_custom_bg = ThemeService::GetThemeProviderForProfile(
                      GetProfile()).HasCustomImage(IDR_THEME_NTP_BACKGROUND);
        dictionary.SetBoolean("hasCustomBackground", has_custom_bg);
        dictionary.SetString("themeId", GetProfile()->GetPrefs()->GetString(prefs::kCurrentThemeID));
        web_ui()->CallJavascriptFunctionUnsafe("ntp.themeChanged", dictionary);
      }
      break;

    default:
      NOTREACHED() << "Unexpected notification type: " << type;
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////
// NewTabHTMLSource

NewTabUI::NewTabHTMLSource::NewTabHTMLSource(Profile* profile)
    : profile_(profile) {
  for (size_t i = 0; i < arraysize(kResources); ++i) {
    AddResource(kResources[i].filename,
                kResources[i].mime_type,
                kResources[i].identifier);
  }
}

std::string NewTabUI::NewTabHTMLSource::GetSource() const {
  return chrome::kChromeUINewTabHost;
}

void NewTabUI::NewTabHTMLSource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  // base::debug::StackTrace().Print();
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::map<std::string, std::pair<std::string, int> >::iterator it =
    resource_map_.find(path);
  if (it != resource_map_.end()) {
    scoped_refptr<base::RefCountedMemory> resource_bytes(
        it->second.second ?
            ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
                it->second.second) :
            new base::RefCountedStaticMemory);
    callback.Run(resource_bytes.get());
    return;
  }

  if (!path.empty() && path[0] != '#') {
    // A path under new-tab was requested; it's likely a bad relative
    // URL from the new tab page, but in any case it's an error.
    NOTREACHED() << path << " should not have been requested on the NTP";
    callback.Run(NULL);
    return;
  }

  content::WebContents* web_contents = wc_getter.Run();
  content::RenderProcessHost* render_host =
      web_contents ? web_contents->GetRenderProcessHost() : nullptr;
  NTPResourceCache::WindowType win_type = NTPResourceCache::GetWindowType(
      profile_, render_host);

  scoped_refptr<base::RefCountedMemory> html_bytes(
      NTPResourceCacheFactory::GetForProfile(profile_)->
      GetNewTabHTML(win_type));

  callback.Run(html_bytes.get());
}

std::string NewTabUI::NewTabHTMLSource::GetMimeType(const std::string& resource)
    const {
  std::map<std::string, std::pair<std::string, int> >::const_iterator it =
      resource_map_.find(resource);
  if (it != resource_map_.end())
    return it->second.first;
  return "text/html";
}

bool NewTabUI::NewTabHTMLSource::ShouldReplaceExistingSource() const {
  return false;
}

bool NewTabUI::NewTabHTMLSource::ShouldAddContentSecurityPolicy() const {
  return false;
}

void NewTabUI::NewTabHTMLSource::AddResource(const char* resource,
                                             const char* mime_type,
                                             int resource_id) {
  DCHECK(resource);
  DCHECK(mime_type);
  resource_map_[std::string(resource)] =
      std::make_pair(std::string(mime_type), resource_id);
}

NewTabUI::NewTabHTMLSource::~NewTabHTMLSource() {}
