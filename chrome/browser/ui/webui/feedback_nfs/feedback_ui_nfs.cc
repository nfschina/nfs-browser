// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/feedback_nfs/feedback_ui_nfs.h"

#include "base/memory/ref_counted_memory.h"
#include "base/memory/singleton.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
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

content::WebUIDataSource* CreateFeedbackUINewHTMLSource(Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kFeedbackNfsHost);

  // source->AddLocalizedString("nfsbrowser", IDS_CDOSBROWSER);
  // source->AddLocalizedString("version", IDS_VERSION);
  // source->AddLocalizedString("core", IDS_CORE);

  source->AddResourcePath("feedback.html",
                          IDR_FEEDBACK_CDOS_HTML);

  source->AddResourcePath("jquery1.8.3.js",
                          IDR_FEEDBACK_CDOS_JS2);

  source->AddResourcePath("feedback.js",
                          IDR_FEEDBACK_CDOS_JS);

  source->AddResourcePath("img/success.png",
                          IDR_FEEDBACK_CDOS_SUCCESS);

  source->AddResourcePath("css/back.css",
                          IDR_FEEDBACK_CDOS_CSS);
  
  source->SetJsonPath("strings.js");

  source->SetDefaultResource(IDR_FEEDBACK_CDOS_HTML);

  return source;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// FeedbackNFSUI
//
///////////////////////////////////////////////////////////////////////////////

FeedbackNFSUI::FeedbackNFSUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = CreateFeedbackUINewHTMLSource(profile);
  content::WebUIDataSource::Add(profile, source);
#if defined(ENABLE_THEMES)
  ThemeSource* theme = new ThemeSource(profile);
  content::URLDataSource::Add(profile, theme);
#endif
}
