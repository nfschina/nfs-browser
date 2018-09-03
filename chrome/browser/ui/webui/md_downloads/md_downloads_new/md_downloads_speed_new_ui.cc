// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/md_downloads/md_downloads_new/md_downloads_speed_new_ui.h"

#include "base/memory/ref_counted_memory.h"
#include "base/memory/singleton.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/download/download_service.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/md_downloads/md_downloads_new/md_downloads_dom_new_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/browser_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserContext;
using content::DownloadManager;
using content::WebContents;

namespace {

content::WebUIDataSource* CreateDownloadsSpeedUINewHTMLSource(Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIDownloadsNewHost);

  source->AddLocalizedString("newtask", IDS_DOWNLOAD_NEWTASK);
  source->AddLocalizedString("website", IDS_DOWNLOAD_WEBSITE);
  source->AddLocalizedString("name", IDS_DOWNLOAD_NAME);
  source->AddLocalizedString("downloadto", IDS_DOWNLOAD_TO);
  source->AddLocalizedString("pleaseenterwebsite", IDS_DOWNLOAD_ENTER_WEBSITE);
  source->AddLocalizedString("pleaseenterfilename", IDS_DOWNLOAD_ENTER_FILENAME);
  source->AddLocalizedString("browse", IDS_DOWNLOAD_BROWSE);
  source->AddLocalizedString("downloadbutton", IDS_DOWNLOAD_BUTTON);
  source->AddLocalizedString("cancelbutton", IDS_CANCEL_BUTTON);

  source->AddLocalizedString("remain", IDS_REMAIN);

  source->AddResourcePath("action_service.html",
                          IDR_MD_DOWNLOADS_NEW_ACTION_SERVICE_HTML);
  source->AddResourcePath("action_service.js",
                          IDR_MD_DOWNLOADS_NEW_ACTION_SERVICE_JS);
  
  source->AddResourcePath("i18n_setup.html", IDR_MD_DOWNLOADS_NEW_I18N_SETUP_HTML);
  source->AddResourcePath("downloads_new.css", IDR_MD_DOWNLOADS_NEW_DOWNLOADS_NEW_CSS);
  source->AddResourcePath("downloads_new.js", IDR_MD_DOWNLOADS_NEW_DOWNLOADS_NEW_JS);
  source->AddResourcePath("shared_style.css",
                          IDR_MD_DOWNLOADS_NEW_SHARED_STYLE_CSS);
  source->SetDefaultResource(IDR_MD_DOWNLOADS_DOWNLOADS_NEW_HTML);

  source->SetJsonPath("strings.js");

  return source;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// MdDownloadsSpeedNewUI
//
///////////////////////////////////////////////////////////////////////////////

MdDownloadsSpeedNewUI::MdDownloadsSpeedNewUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  DownloadManager* dlm = BrowserContext::GetDownloadManager(profile);

  handler_ = new MdDownloadsDOMNewHandler(dlm, web_ui);
  web_ui->OverrideTitle(l10n_util::GetStringUTF16(IDS_NEW_DOWNLOAD_TITLE));
  web_ui->AddMessageHandler(handler_);

  content::WebUIDataSource* source = CreateDownloadsSpeedUINewHTMLSource(profile);
  content::WebUIDataSource::Add(profile, source);
#if defined(ENABLE_THEMES)
  ThemeSource* theme = new ThemeSource(profile);
  content::URLDataSource::Add(profile, theme);
#endif
}

// static
base::RefCountedMemory* MdDownloadsSpeedNewUI::GetFaviconResourceBytes(
    ui::ScaleFactor scale_factor) {
  return ResourceBundle::GetSharedInstance().
      LoadDataResourceBytesForScale(IDR_DOWNLOADS_FAVICON, scale_factor);
}

void MdDownloadsSpeedNewUI::RenderViewReused(
    content::RenderViewHost* render_view_host) {
  handler_->RenderViewReused(render_view_host);
}
