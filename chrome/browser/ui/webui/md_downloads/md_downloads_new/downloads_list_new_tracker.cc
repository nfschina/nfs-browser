// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/md_downloads/md_downloads_new/downloads_list_new_tracker.h"

#include <iterator>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/bind_helpers.h"
#include "base/i18n/rtl.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/value_conversions.h"
#include "base/values.h"
#include "chrome/browser/download/all_download_item_notifier.h"
#include "chrome/browser/download/download_crx_util.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/download_item_model_speed.h"
#include "chrome/browser/download/download_query.h"
#include "chrome/browser/extensions/api/downloads/downloads_api.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_system.h"
#include "net/base/filename_util.h"
#include "third_party/icu/source/i18n/unicode/datefmt.h"
#include "ui/base/l10n/time_format.h"

using content::BrowserContext;
using content::DownloadItem;
using content::DownloadManager;

using DownloadVector = DownloadManager::DownloadVector;

// The max selected download directorys
//const int kMaxSelectedDownloadDirectorys = 3;

DownloadsListNewTracker::DownloadsListNewTracker(
    DownloadManager* download_manager,
    content::WebUI* web_ui)
  : main_notifier_(download_manager, this),
    web_ui_(web_ui) {
}

DownloadsListNewTracker::~DownloadsListNewTracker() {}

void DownloadsListNewTracker::OnLoad() {
  if (info_.get()) {
    web_ui_->CallJavascriptFunctionUnsafe(
      "downloadPromptd",
      base::StringValue(info_->site_url.spec()), 
	  base::StringValue(info_->save_info->suggested_name),
      base::StringValue(info_->save_info->file_path.value()),
      base::StringValue(info_->auth_credential.username()),
      base::StringValue(info_->auth_credential.password()));
  } else {
    base::FilePath default_download_directory = 
      GetMainNotifierManager()->GetDefaultDownloadPath();
    web_ui_->CallJavascriptFunctionUnsafe(
      "downloadPromptd",
      base::StringValue(""), 
      base::StringValue(""),
      base::StringValue(default_download_directory.value()),
      base::StringValue(""),
      base::StringValue(""));
  }

  Profile* profile = Profile::FromBrowserContext(
    GetMainNotifierManager()->GetBrowserContext());
  if (profile) {
    std::vector<base::FilePath> paths = 
      profile->download_selected_directorys();
    std::vector<base::FilePath>::reverse_iterator iter;
    for (iter = paths.rbegin(); iter != paths.rend(); iter++) {
      base::FilePath path = *iter;
      web_ui_->CallJavascriptFunctionUnsafe(
        "downloadDirectory",
        base::StringValue(path.value()));
    }

    /*size_t empty_count = kMaxSelectedDownloadDirectorys - paths.size();
    for (size_t i = 0; i < empty_count; i++) {
      web_ui_->CallJavascriptFunctionUnsafe(
        "downloadDirectory",
        base::StringValue(""));
    }*/
  }
}

DownloadManager* DownloadsListNewTracker::GetMainNotifierManager() const {
  return main_notifier_.GetManager();
}

DownloadManager* DownloadsListNewTracker::GetOriginalNotifierManager() const {
  return original_notifier_ ? original_notifier_->GetManager() : nullptr;
}

void DownloadsListNewTracker::OnDownloadPromptd(content::DownloadManager* manager, 
                                                content::DownloadCreateInfo* info) {
  info_.reset(new content::DownloadCreateInfo);
  info_->site_url = info->url_chain.back();
  info_->save_info->suggested_name = info->save_info->suggested_name;
  info_->save_info->file_path = info->save_info->file_path;
  info_->content_disposition = info->content_disposition;
  info_->mime_type = info->mime_type;
  info_->referrer_url = info->referrer_url;
  info_->tab_url = info->tab_url;
  info_->method = info->method;
  info_->pack_url = info->pack_url;
  info_->auth_credential = info->auth_credential;
}

DownloadsListNewTracker::DownloadsListNewTracker(
    DownloadManager* download_manager,
    content::WebUI* web_ui,
    base::Callback<bool(const DownloadItem&)> should_show)
  : main_notifier_(download_manager, this),
    web_ui_(web_ui) {
}

