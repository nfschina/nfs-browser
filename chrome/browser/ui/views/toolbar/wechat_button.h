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

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_WECHAT_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_WECHAT _BUTTON_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/views/controls/button/image_button.h"

class Browser;

class WechatButton : public views::ImageButton{
 public:
  WechatButton(Browser* browser,
             views::ButtonListener* listener);
  ~WechatButton() override;

 private:
  const char* GetClassName() const override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  Browser* browser_;

  content::NotificationRegistrar registrar_;


  base::WeakPtrFactory<WechatButton> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WechatButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_WECHAT_BUTTON_H_