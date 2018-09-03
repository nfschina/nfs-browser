// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_PROGRESS_MANAGER_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_PROGRESS_MANAGER_H_

#include <memory>
#include <set>
#include <vector>
#include "base/synchronization/lock.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"


class DownloadItemProgress;

class DownloadProgressManager {
 public:
  struct ProgressInfo
  {
    int progress;
    ProgressInfo();
    ~ProgressInfo();
  };

  static DownloadProgressManager* GetInstance();

  void AddDownload(content::DownloadItem* download);
  void RemoveDownload(content::DownloadItem* download);
  void OnProgressUpdate();
  base::OneShotTimer& GetTimer() { return timer_; }

 private:
  int last_progress_;
  std::vector<std::unique_ptr<DownloadItemProgress> > downloads_;
  base::Lock downloads_lock_;
  base::OneShotTimer timer_;
  static DownloadProgressManager* download_progress_manager_ ;

   DownloadProgressManager();
  ~DownloadProgressManager();

  bool IsAllItemsStateOf(content::DownloadItem::DownloadState state);
  bool IsDownloadComplete();

  DISALLOW_COPY_AND_ASSIGN(DownloadProgressManager);
};

//class DownloadItemProgress;
class DownloadItemProgress : public content::DownloadItem::Observer {
 public:
  DownloadItemProgress(DownloadProgressManager* manager,
        content::DownloadItem* download);

  ~DownloadItemProgress() override;

   // DownloadItem::Observer methods
  void OnDownloadUpdated(content::DownloadItem* download) override;
  // void OnDownloadOpened(DownloadItem* download) override;
  void OnDownloadDestroyed(content::DownloadItem* download) override;

  content::DownloadItem* download() { return download_; }


 private:
  DownloadProgressManager* manager_;
  content::DownloadItem* download_;
  content::DownloadItem::DownloadState last_state_;
  //last complete percent.
  int last_percent_;

  DISALLOW_COPY_AND_ASSIGN(DownloadItemProgress);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_PROGRESS_MANAGER_H_
