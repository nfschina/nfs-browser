// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/theme_gallery/theme_gallery_handler.h"

#include "base/environment.h"
#include "base/path_service.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/profile_chooser_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_system.h"

#if defined(OS_POSIX)
#define STRING_LITERAL(x) x
typedef std::string string16;
#elif defined(OS_WIN)
#define STRING_LITERAL(x) L ## x
typedef std::wstring string16;
#endif

using extensions::Extension;

namespace {
  // const char kThemeDir[] = "themes";
  // const char kThemePostfix[] = ".crx";
  const char kDefaultThemeID[] = "default";
}

ThemeGalleryHandler::ThemeGalleryHandler(content::WebUI* web_ui) {
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
      content::Source<ThemeService>(
          ThemeServiceFactory::GetForProfile(Profile::FromWebUI(web_ui))));
}

ThemeGalleryHandler::~ThemeGalleryHandler() {
  registrar_.RemoveAll();
}

void ThemeGalleryHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "installTheme", base::Bind(&ThemeGalleryHandler::HandleInstallTheme,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getCurrentTheme", base::Bind(&ThemeGalleryHandler::HandleGetCurrentTheme,
                                         base::Unretained(this)));
}

void ThemeGalleryHandler::Observe(int type,
                 const content::NotificationSource& source,
                 const content::NotificationDetails& details) {
  std::string themeID;
  switch (type) {
    case chrome::NOTIFICATION_BROWSER_THEME_CHANGED:
/*
      //Not trigger in theme gallery.
      if (!installing_) {
        return;
      }
      //TODO:filter non theme gallery install
      installing_ = false;
      themeID = GetProfile()->GetPrefs()->GetString(prefs::kCurrentThemeID);
      // printf("@@ install complete, id=%s\n", themeID.c_str());
      web_ui()->CallJavascriptFunctionUnsafe("theme_gallery.themeInstallComplete");
      break;
*/
      themeID = GetProfile()->GetPrefs()->GetString(prefs::kCurrentThemeID);
      if (themeID.empty()) {
        themeID = std::string("default");
      }
      web_ui()->CallJavascriptFunctionUnsafe(
          "theme_gallery.themeInstallComplete", base::StringValue(themeID));
      break;

    default:
      NOTREACHED() << "Unexpected notification type: " << type;
      break;
  }
}

void ThemeGalleryHandler::HandleInstallTheme(const base::ListValue* args) {
  std::string themeID;
  if (!args->GetString(0, &themeID))
    return ;

  ExtensionService* service =
          extensions::ExtensionSystem::Get(GetProfile())->extension_service();
  const Extension* extension =
          service ? service->GetExtensionById(themeID, false) : nullptr;

  if (themeID == kDefaultThemeID || extension) { //TODO:filter
    SetTheme(themeID);
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "theme_gallery.installThemeFromServer", base::StringValue(themeID));

/*
  scoped_refptr<extensions::CrxInstaller> installer(extensions::CrxInstaller::CreateSilent(
          extensions::ExtensionSystem::Get(GetBrowser()->profile())->extension_service()));
  base::FilePath crx_path;
  if (PathService::Get(chrome::DIR_APP, &crx_path)) {
    crx_path = crx_path.AppendASCII(kThemeDir);
    std::string themeCrx = themeID.append(kThemePostfix);
    crx_path = crx_path.AppendASCII(themeCrx);
    // printf("###install crx_path=%s\n", crx_path.AsUTF8Unsafe().c_str());
    installer->set_allow_silent_install(true);
    installer->set_install_immediately(true);
    installer->InstallCrx(crx_path);
  }
*/
}

void ThemeGalleryHandler::HandleGetCurrentTheme(const base::ListValue* args) {
  std::string themeID =
      GetProfile()->GetPrefs()->GetString(prefs::kCurrentThemeID);
  if (themeID.empty()) {
    themeID = kDefaultThemeID;  //default theme
  }
  web_ui()->CallJavascriptFunctionUnsafe(
      "theme_gallery.setCurrentTheme", base::StringValue(themeID));
}

Browser* ThemeGalleryHandler::GetBrowser() {
  DCHECK(web_ui());
  content::WebContents* contents = web_ui()->GetWebContents();
  DCHECK(contents);
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  return browser;
}

Profile* ThemeGalleryHandler::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}

void ThemeGalleryHandler::SetTheme(std::string themeID) {
  content::NotificationService* service =
      content::NotificationService::current();
  service->Notify(chrome::NOTIFICATION_THEME_GALLERY_CHANGE,
                  content::Source<ThemeGalleryHandler>(this),
                  content::Details<std::string>(&themeID));
}
