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
#include "chrome/browser/ui/views/toolbar/wechat_button.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/resource/resource_bundle.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu_button.h"
#include "extensions/browser/pre_install/preinstall_crx_info.h"

WechatButton::WechatButton(Browser* browser,
                       views::ButtonListener* listener)
    : ImageButton(listener),
    browser_(browser),
    weak_ptr_factory_(this) {
}

WechatButton::~WechatButton() {
}

const char* WechatButton::GetClassName() const{
  return "WechatButton";
}

void WechatButton::OnMouseReleased(const ui::MouseEvent& event) {
  // #if defined(OS_POSIX)
  // --app-id=ibcngnihipkacaaeclcegicibajenpmi
  //   char* cmd = "/opt/nfsbrowser/nfsbrowser--"
  //   system()
  LOG(INFO) << browser_;
}
