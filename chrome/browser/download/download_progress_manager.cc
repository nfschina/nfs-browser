#include "chrome/browser/download/download_progress_manager.h"

#include "chrome/browser/chrome_notification_types.h"

using  content::DownloadItem;

namespace {
  const int kDownloadProgressRateMs = 30;
}

DownloadItemProgress::DownloadItemProgress(DownloadProgressManager* manager,
     DownloadItem* download)
  : manager_(manager),
  download_(download),
  last_state_(DownloadItem::MAX_DOWNLOAD_STATE),
  last_percent_(-1) {
  download_->AddObserver(this);
}

DownloadItemProgress::~DownloadItemProgress() {
  download_->RemoveObserver(this);
}

void DownloadItemProgress::OnDownloadUpdated(DownloadItem* download) {
  DCHECK_EQ(download_, download);

  DownloadItem::DownloadState state = download->GetState();
  int percent = download->PercentComplete();
  if ((state != DownloadItem::IN_PROGRESS && last_state_ == state) ||
        (state == DownloadItem::IN_PROGRESS &&
         last_state_ == state && last_percent_ == percent)) {
    return;
  }
  last_state_ = state;
  last_percent_ = percent;
  base::OneShotTimer& timer = manager_->GetTimer();

  switch (state) {
    case DownloadItem::IN_PROGRESS:
    case DownloadItem::COMPLETE:
    case DownloadItem::PAUSING:
    case DownloadItem::INTERRUPTED:
    case DownloadItem::CANCELLED:
      if (download->GetState() == DownloadItem::IN_PROGRESS && timer.IsRunning()) {
        return;
        }
      timer.Start(FROM_HERE,  base::TimeDelta::FromMilliseconds(kDownloadProgressRateMs),
                  base::Bind(&DownloadProgressManager::OnProgressUpdate, base::Unretained(manager_)));
      break;

    default:
      //NOTREACHED();
      break;
  }
}

void DownloadItemProgress::OnDownloadDestroyed(DownloadItem* download) {
  manager_->RemoveDownload(download);
}

DownloadProgressManager* DownloadProgressManager::download_progress_manager_ = nullptr;

DownloadProgressManager* DownloadProgressManager::GetInstance() {
  if (download_progress_manager_) {
    return download_progress_manager_;
  }

  return download_progress_manager_ = new DownloadProgressManager();
}

//############################################################################
DownloadProgressManager::ProgressInfo::ProgressInfo()
  : progress(-1) {
}

DownloadProgressManager::ProgressInfo::~ProgressInfo() {
}

DownloadProgressManager::DownloadProgressManager()
  : last_progress_(-1) {
}

DownloadProgressManager::~DownloadProgressManager() {
}

void DownloadProgressManager::AddDownload(DownloadItem* download) {
  std::unique_ptr<DownloadItemProgress> item(new DownloadItemProgress(this, download));
  base::AutoLock lock(downloads_lock_);
  downloads_.push_back(std::move(item));
}

void DownloadProgressManager::RemoveDownload(DownloadItem* download) {
  base::AutoLock lock(downloads_lock_);
  for (size_t i=0; i < downloads_.size(); i++) {
    if (download == downloads_[i]->download()) {
      downloads_.erase(downloads_.begin() + i);
      break;
    }
  }
  timer_.Start(FROM_HERE,  base::TimeDelta::FromMilliseconds(kDownloadProgressRateMs),
                  base::Bind(&DownloadProgressManager::OnProgressUpdate, base::Unretained(this)));
}

bool DownloadProgressManager::IsAllItemsStateOf(DownloadItem::DownloadState state) {
  int count = downloads_.size();
  for (int i = 0; i < count; i++) {
    DownloadItem* download_item = downloads_[i]->download();
    if (download_item->GetState() != state) {
      return false;
    }
  }
  return true;
}

bool DownloadProgressManager::IsDownloadComplete() {
  int count = downloads_.size();
  for (int i = 0; i < count; i++) {
    DownloadItem* download_item = downloads_[i]->download();
    if (download_item->GetState() == DownloadItem::IN_PROGRESS
          || download_item->GetState() == DownloadItem::PAUSING) {
      return false;
    }
  }
  return true;
}

void DownloadProgressManager::OnProgressUpdate() {
  int progress = 0;
  int percent = 0;
  int show_progress = -1;
  int count = downloads_.size();
  if (!count) {
    show_progress = -1;
  } else {
    base::AutoLock lock(downloads_lock_);
    for (int i = 0; i < count; i++) {
      DownloadItem* download_item = downloads_[i]->download();
      if (download_item) {
        DownloadItem::DownloadState state = download_item->GetState();
        if (state == DownloadItem::IN_PROGRESS ||
              state == DownloadItem::COMPLETE ||
              state == DownloadItem::PAUSING ) {
          progress += ((percent = download_item->PercentComplete()) > 0 ? percent : 0);
        }
      } else {
        NOTREACHED();
      }
    }
    progress /= count;

    show_progress = progress - progress % 25;

    if (IsDownloadComplete()) {
      if (IsAllItemsStateOf(DownloadItem::CANCELLED)) {
        show_progress = -1;
      } else {
        show_progress = 100;
      }
      downloads_.clear();
    }
  }

  if (last_progress_ != show_progress) {
    last_progress_ = show_progress;
    //Notify UI to update download animation.
     ProgressInfo info;
     info.progress = last_progress_;
     content::NotificationService* service =
      content::NotificationService::current();
      service->Notify(chrome::NOTIFICATION_DOWNLOAD_PROGRESS,
                      content::Source<DownloadProgressManager>(this),
                      content::Details<ProgressInfo>(&info));
  }

  if (IsDownloadComplete()) {
    last_progress_ = -1;
  }
}
