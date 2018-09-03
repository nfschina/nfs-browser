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

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_DOWNLOAD_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_DOWNLOAD _BUTTON_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/views/controls/button/image_button.h"

class Browser;

class DownloadButton : public views::ImageButton,
					public content::NotificationObserver {
 public:
  DownloadButton(Browser* browser,
             views::ButtonListener* listener);
  ~DownloadButton() override;

 private:
  const char* GetClassName() const override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void InitDownloadManager();

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  //views::View overrides
  void ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) override;
  void OnThemeChanged() override;

  void SetDark(bool dark) { is_dark_ = dark; }

  bool IsDark() { return is_dark_; }

  void LoadImage();

  Browser* browser_;

  content::NotificationRegistrar registrar_;

  int progress_;

  //is dark button
  bool is_dark_;

  base::WeakPtrFactory<DownloadButton> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_DOWNLOAD_BUTTON_H_