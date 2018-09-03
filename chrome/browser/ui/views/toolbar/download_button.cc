/**
 * Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, CAS
 *
 * This software is published by the license of CPU-OS Licence, you can use and
 * distribute this software under this License. See CPU-OS License for more detail.
 *
 * You should have received a copy of CPU-OS License. If not, please contact us
 * by email <support_os@cpu-os.ac.cn>
 *
**/

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/download/chrome_download_manager_delegate.h"
#include "chrome/browser/download/download_progress_manager.h"
#include "chrome/browser/ui/views/toolbar/download_button.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/views/download/download_feedback_view_nfs.h"
#include "chrome/browser/themes/theme_properties.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_service.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu_button.h"

DownloadButton::DownloadButton(Browser* browser,
                       views::ButtonListener* listener)
    : ImageButton(listener),
    browser_(browser),
    progress_(-1),
    is_dark_(false),
    weak_ptr_factory_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_DOWNLOAD_FEEDBACK,
      content::Source<ChromeDownloadManagerDelegate>(nullptr));
  registrar_.Add(this, chrome::NOTIFICATION_DOWNLOAD_PROGRESS,
      content::Source<ChromeDownloadManagerDelegate>(nullptr));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&DownloadButton::InitDownloadManager, weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(3));
}

DownloadButton::~DownloadButton() {
  registrar_.Remove(
        this, chrome::NOTIFICATION_DOWNLOAD_FEEDBACK,
        content::Source<ChromeDownloadManagerDelegate>(nullptr));
   registrar_.Remove(
        this, chrome::NOTIFICATION_DOWNLOAD_PROGRESS,
        content::Source<ChromeDownloadManagerDelegate>(nullptr));
}

const char* DownloadButton::GetClassName() const{
  return "DownloadButton";
}

void DownloadButton::OnMouseReleased(const ui::MouseEvent& event) {
  chrome::ShowDownloads(browser_);

  int id_download_n = IsDark() ? IDD_DOWNLOAD_N_DARK : IDD_DOWNLOAD_N;
  if (progress_ == 100 ) {
     progress_ = -1;
     ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      SetImage(views::Button::STATE_NORMAL,
                             rb.GetImageNamed(id_download_n).ToImageSkia());
  }
}

void DownloadButton::InitDownloadManager() {
  // huk
 // chrome::ShowDownloads(browser_, false);
}

void DownloadButton::Observe(int type,
                 const content::NotificationSource& source,
                 const content::NotificationDetails& details) {
  DownloadFeedbackView::DownloadInfo* info = nullptr;
  DownloadProgressManager::ProgressInfo* progressInfo = nullptr;
  switch (type) {
    case chrome::NOTIFICATION_DOWNLOAD_FEEDBACK:
      info = content::Details<DownloadFeedbackView::DownloadInfo>(details).ptr();
      if (info && info->browser == browser_) {
        views::View* anchor_view = this;
        if (!anchor_view->visible() )   {
          BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
          anchor_view = browser_view->toolbar()->app_menu_button();
        }

        DownloadFeedbackView::ShowBubble(anchor_view, gfx::Rect(), nullptr, *info);
      }
      break;

    case chrome::NOTIFICATION_DOWNLOAD_PROGRESS:
      progressInfo = content::Details< DownloadProgressManager::ProgressInfo>(details).ptr();
      if(!progressInfo)
        return;
      progress_ = progressInfo->progress;
      LoadImage();
      break;

    default:
      NOTREACHED() << "Unexpected notification type: " << type;
      break;
  }
}

void DownloadButton::ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) {
  if(details.is_add && details.child == this) {
    const ui::ThemeProvider* tp = GetThemeProvider();
    SetDark(tp && tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK));
    LoadImage();
  }
}

void DownloadButton::OnThemeChanged() {
  const ui::ThemeProvider* tp = GetThemeProvider();
  SetDark(tp && tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK));
  LoadImage();
}

void DownloadButton::LoadImage() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int image_id = IsDark() ? IDD_DOWNLOAD_N_DARK : IDD_DOWNLOAD_N;
  int id_download_h = IsDark() ? IDD_DOWNLOAD_H_DARK : IDD_DOWNLOAD_H;
  switch (progress_) {
    case 0:
      image_id = IsDark() ? IDD_DOWNLOAD_PERCENT_0_DARK : IDD_DOWNLOAD_PERCENT_0;
      break;

    case 25:
      image_id = IsDark() ? IDD_DOWNLOAD_PERCENT_25_DARK : IDD_DOWNLOAD_PERCENT_25;
      break;

    case 50:
      image_id = IsDark() ? IDD_DOWNLOAD_PERCENT_50_DARK : IDD_DOWNLOAD_PERCENT_50;
      break;

    case 75:
      image_id = IsDark() ? IDD_DOWNLOAD_PERCENT_75_DARK : IDD_DOWNLOAD_PERCENT_75;
      break;

    case 100:
      image_id = IsDark() ? IDD_DOWNLOAD_PERCENT_100_DARK : IDD_DOWNLOAD_PERCENT_100;
      break;

    default:
    break;
  }
  SetImage(views::Button::STATE_NORMAL, rb.GetImageNamed(image_id).ToImageSkia());
  SetImage(views::Button::STATE_HOVERED, rb.GetImageNamed(id_download_h).ToImageSkia());
}
