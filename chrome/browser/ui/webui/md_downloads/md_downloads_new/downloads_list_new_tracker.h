// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_DOWNLOADS_LIST_NEW_TRACKER_H_
#define CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_DOWNLOADS_LIST_NEW_TRACKER_H_

#include <stddef.h>

#include <memory>
#include <set>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/download/all_download_item_notifier.h"
#include "content/public/browser/download_item.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace content {
class DownloadManager;
class WebUI;
}

// A class that tracks all downloads activity and keeps a sorted representation
// of the downloads as chrome://downloads wants to display them.
class DownloadsListNewTracker : public AllDownloadItemNotifier::Observer {
 public:
  DownloadsListNewTracker(content::DownloadManager* download_manager,
                          content::WebUI* web_ui);
  ~DownloadsListNewTracker() override;

  // Init the download new info
  void OnLoad();

  content::DownloadManager* GetMainNotifierManager() const;
  content::DownloadManager* GetOriginalNotifierManager() const;

  // AllDownloadItemNotifier::Observer:
  void OnDownloadPromptd(content::DownloadManager* manager, 
                         content::DownloadCreateInfo* info) override;

  content::DownloadCreateInfo* GetCreateInfo() { return info_.get(); }

 protected:
  // Testing constructor.
  DownloadsListNewTracker(content::DownloadManager* download_manager,
                          content::WebUI* web_ui,
                          base::Callback<bool(const content::DownloadItem&)>);

  // Exposed for testing.
  bool IsIncognito(const content::DownloadItem& item) const;

 private:

  AllDownloadItemNotifier main_notifier_;
  std::unique_ptr<AllDownloadItemNotifier> original_notifier_;

  // The WebUI object corresponding to the page we care about.
  content::WebUI* const web_ui_;

  // save the download create info
  std::unique_ptr<content::DownloadCreateInfo> info_;

  // When this is true, all changes to downloads that affect the page are sent
  // via JavaScript.
  bool sending_updates_ = false;

  // The number of items sent to the page so far.
  size_t sent_to_page_ = 0u;

  // The maximum number of items sent to the page at a time.
  size_t chunk_size_ = 20u;

  DISALLOW_COPY_AND_ASSIGN(DownloadsListNewTracker);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_DOWNLOADS_LIST_NEW_TRACKER_H_
