// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/theme_gallery/theme_gallery_ui.h"

#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_gallery/theme_gallery_handler.h"
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


ThemeGalleryUI::ThemeGalleryUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  web_ui->AddMessageHandler(new ThemeGalleryHandler(web_ui));

  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kThemeGalleryHost);
  html_source->DisableReplaceExistingSource();
  html_source->DisableContentSecurityPolicy();

  html_source->AddLocalizedString("themegallerytitle", IDS_THEMES_GALLERY_TITLE);
  html_source->AddLocalizedString("classictheme", IDS_CLASSIC_THEMES);
  html_source->AddLocalizedString("usetheme", IDS_USE_THEME);
  html_source->AddLocalizedString("wallpaperskin", IDS_WALLPAPER_SKIN);

  html_source->AddResourcePath("main.html", IDR_THEME_GALLERY_MAIN);
  html_source->AddResourcePath("skin.js", IDR_THEME_GALLERY_JS);
  html_source->AddResourcePath("skin.css", IDR_THEME_GALLERY_CSS);
  html_source->AddResourcePath("img/0.jpg", IDR_THEME_GALLERY_IMG_0);
  html_source->AddResourcePath("img/1.jpg", IDR_THEME_GALLERY_IMG_1);
  html_source->AddResourcePath("img/2.jpg", IDR_THEME_GALLERY_IMG_2);
  html_source->AddResourcePath("img/3.jpg", IDR_THEME_GALLERY_IMG_3);
  html_source->AddResourcePath("img/4.jpg", IDR_THEME_GALLERY_IMG_4);
  html_source->AddResourcePath("img/5.jpg", IDR_THEME_GALLERY_IMG_5);
  html_source->AddResourcePath("img/6.jpg", IDR_THEME_GALLERY_IMG_6);
  html_source->AddResourcePath("img/7.jpg", IDR_THEME_GALLERY_IMG_7);
  html_source->AddResourcePath("img/8.jpg", IDR_THEME_GALLERY_IMG_8);
  html_source->AddResourcePath("img/defaultskin.png", IDR_THEME_GALLERY_IMG_DEFAULT_INDI);
  html_source->AddResourcePath("img/loading.png", IDR_THEME_GALLERY_IMG_LOADING);
  html_source->AddResourcePath("img/favicon.ico", IDR_THEME_GALLERY_FAVICON);
  html_source->SetDefaultResource(IDR_THEME_GALLERY_MAIN);
  html_source->SetJsonPath("strings.js");

  content::WebUIDataSource::Add(profile, html_source);
}

ThemeGalleryUI::~ThemeGalleryUI() {
}

// // static
// base::RefCountedMemory* ThemeGalleryUI::GetFaviconResourceBytes(
//       ui::ScaleFactor scale_factor) {
//   return ResourceBundle::GetSharedInstance().
//       LoadDataResourceBytesForScale(IDR_THEME_GALLERY_FAVICON, scale_factor);
// }

Profile* ThemeGalleryUI::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}
