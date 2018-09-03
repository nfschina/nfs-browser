// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/download/chrome_download_manager_delegate.h"
#include "chrome/browser/download/download_progress_manager.h"
#include "chrome/browser/ui/views/toolbar/app_manager_button.h"
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

AppManagerButton::AppManagerButton(Browser* browser,
                       views::ButtonListener* listener)
    : ImageButton(listener),
    browser_(browser),
    is_dark_(false),
    weak_ptr_factory_(this) {  
}

AppManagerButton::~AppManagerButton() {  
}

const char* AppManagerButton::GetClassName() const{
  return "AppManagerButton";
}

void AppManagerButton::OnMouseReleased(const ui::MouseEvent& event) {
  chrome::ShowAppManager(browser_);  
}

void AppManagerButton::ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) {
  if(details.is_add && details.child == this) {
    const ui::ThemeProvider* tp = GetThemeProvider();
    SetDark(tp && tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK));
    LoadImage();
  }
}

void AppManagerButton::OnThemeChanged() {
  const ui::ThemeProvider* tp = GetThemeProvider();
  SetDark(tp && tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK));
  LoadImage();
}

void AppManagerButton::LoadImage() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int image_id = IsDark() ? IDD_APP_MANAGER_N : IDD_APP_MANAGER_N;
  int image_h_id = IsDark() ? IDD_APP_MANAGER_H : IDD_APP_MANAGER_H; 
  SetImage(views::Button::STATE_NORMAL, rb.GetImageNamed(image_id).ToImageSkia());
  SetImage(views::Button::STATE_HOVERED, rb.GetImageNamed(image_h_id).ToImageSkia());
}
