// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/app_manager/app_manager_ui.h"

#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/app_manager/app_manager_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/chrome_unscaled_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/resources/grit/webui_resources.h"
#include "ui/resources/grit/webui_resources_nfs.h"


AppManagerUI::AppManagerUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  web_ui->AddMessageHandler(new AppManagerHandler(web_ui));

  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kAppManagerHost);

  html_source->AddLocalizedString("app_manager_title", IDS_APP_MANAGER_TITLE);
  html_source->AddLocalizedString("installed_app", IDS_APP_MANAGER_INSTALLED_APP);
  html_source->AddLocalizedString("recommend", IDS_APP_MANAGER_RECOMMEND);
  html_source->AddLocalizedString("install", IDS_APP_MANAGER_INSTALL);
  html_source->AddLocalizedString("installed", IDS_APP_MANAGER_INSTALLED);
  html_source->AddLocalizedString("installing", IDS_APP_MANAGER_INSTALLING);
  html_source->AddLocalizedString("setting", IDS_APP_MANAGER_SETTING);
  html_source->AddLocalizedString("disable", IDS_APP_MANAGER_DISABLE);
  html_source->AddLocalizedString("enable", IDS_APP_MANAGER_ENABLE);
  html_source->AddLocalizedString("uninstall", IDS_APP_MANAGER_UNINSTALL);
  html_source->AddLocalizedString("more", IDS_APP_MANAGER_MORE);
  html_source->AddLocalizedString("adblock_name", IDS_APP_MANAGER_ADBLOCK_NAME);
  html_source->AddLocalizedString("adblock_description", IDS_APP_MANAGER_ADBLOCK_DESCRIPTION);
  html_source->AddLocalizedString("readingMode_name", IDS_APP_MANAGER_READINGMODE_NAME);
  html_source->AddLocalizedString("readingMode_description", IDS_APP_MANAGER_READINGMODE_DESCRIPTION);
  html_source->AddLocalizedString("weChat_name", IDS_APP_MANAGER_WECHAT_NAME);
  html_source->AddLocalizedString("weChat_description", IDS_APP_MANAGER_WECHAT_DESCRIPTION);
  html_source->AddLocalizedString("activeX_name", IDS_APP_MANAGER_ACTIVEX_NAME);
  html_source->AddLocalizedString("activeX_description", IDS_APP_MANAGER_ACTIVEX_DESCRIPTION);
  html_source->AddLocalizedString("smoothScroll_name", IDS_APP_MANAGER_SMOOTHSCROLL_NAME);
  html_source->AddLocalizedString("smoothScroll_description", IDS_APP_MANAGER_SMOOTHSCROLL_DESCRIPTION);
  html_source->AddLocalizedString("pepperFlash_name", IDS_APP_MANAGER_PEPPERFLASH_NAME);
  html_source->AddLocalizedString("pepperFlash_description", IDS_APP_MANAGER_PEPPERFLASH_DESCRIPTION);
  html_source->AddLocalizedString("mousegesture_name", IDS_APP_MANAGER_MOUSEGESTURE_NAME);
  html_source->AddLocalizedString("mousegesture_description", IDS_APP_MANAGER_MOUSEGESTURE_DESCRIPTION);

  html_source->AddResourcePath("app_manager.html", IDR_APP_MANAGER_MAIN);
  html_source->AddResourcePath("app_manager.js", IDR_APP_MANAGER_JS);
  html_source->AddResourcePath("app_manager.css", IDR_APP_MANAGER_CSS);
  html_source->AddResourcePath("img/add.png", IDR_APP_MANAGER_IMG_ADD);
  html_source->AddResourcePath("img/adblock.png", IDR_APP_MANAGER_IMG_ADBLOCK);
  html_source->AddResourcePath("img/ReadMode.png", IDR_APP_MANAGER_IMG_READMODE);
  html_source->AddResourcePath("img/activeX.png", IDR_APP_MANAGER_IMG_ACTIVEX);
  html_source->AddResourcePath("img/smoothScroll.png", IDR_APP_MANAGER_IMG_SMOOTHSCROLL);
  html_source->AddResourcePath("img/weChat.png", IDR_APP_MANAGER_IMG_WECHAT);
  html_source->AddResourcePath("img/flash.png", IDR_APP_MANAGER_IMG_FLASH);
  html_source->AddResourcePath("img/mousegesture.png", IDR_APP_MANAGER_IMG_MOUSEGESTURE);  
  html_source->SetDefaultResource(IDR_APP_MANAGER_MAIN);
  html_source->SetJsonPath("strings.js");

  content::WebUIDataSource::Add(profile, html_source);
}

AppManagerUI::~AppManagerUI() {
}

Profile* AppManagerUI::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}
