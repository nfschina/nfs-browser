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

#include "chrome/browser/ui/views/toolbar/revocation_button.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/command_updater.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"

namespace {
  const int kLeftPart = 36;
}

RevocationButton::RevocationButton(Browser* browser,
                       views::ButtonListener* listener,
                       ui::MenuModel* model)
    : ToolbarButton(browser->profile(), listener, model),
    last_on_left_(false),
    browser_(browser) {
  // set_ink_drop_delegate(nullptr);
  set_has_ink_drop_action_on_click(false);
}

RevocationButton::~RevocationButton() {
}

const char* RevocationButton::GetClassName() const {
  return "RevocationButton";
}

void RevocationButton::NotifyClick(const ui::Event& event) {
  ToolbarButton::NotifyClick(event);
  DCHECK(event.IsMouseEvent());
  if (!event.IsMouseEvent())  {
    return;
  }

  if (event.AsMouseEvent()->x() < kLeftPart)  {
    browser_->command_controller()->command_updater()->ExecuteCommand(IDC_RESTORE_TAB);
  } else {
    ShowDropDownMenu(ui::GetMenuSourceTypeForEvent(event));
  }
}

void RevocationButton::OnMouseEntered(const ui::MouseEvent& event) {
  last_on_left_ = IsLeftPart(event);
  SetRealImage(last_on_left_);
  ToolbarButton::OnMouseEntered(event);
}

void RevocationButton::OnMouseMoved(const ui::MouseEvent& event) {
  bool on_left = IsLeftPart(event);
  if (on_left !=last_on_left_) {
    last_on_left_ = on_left;
    SetRealImage(on_left);
  }
  ToolbarButton::OnMouseMoved(event);
}

void RevocationButton::SetRealImage(bool left) {
  const ui::ThemeProvider* tp = GetThemeProvider();
  bool dark = tp && tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (left) {
     SetImage(views::Button::STATE_HOVERED,
                  *(rb.GetImageNamed(dark ? IDD_REVOCATION_LEFT_H_DARK : IDD_REVOCATION_LEFT_H).ToImageSkia()));
     SetImage(views::Button::STATE_PRESSED,
                  *(rb.GetImageNamed(dark ? IDD_REVOCATION_LEFT_P_DARK : IDD_REVOCATION_LEFT_P).ToImageSkia()));
  } else {
    SetImage(views::Button::STATE_HOVERED,
                  *(rb.GetImageNamed(dark ? IDD_REVOCATION_H_DARK : IDD_REVOCATION_H).ToImageSkia()));
    SetImage(views::Button::STATE_PRESSED,
                  *(rb.GetImageNamed(dark ? IDD_REVOCATION_P_DARK : IDD_REVOCATION_P).ToImageSkia()));
  }
}

bool RevocationButton::IsLeftPart(const ui::MouseEvent& event) {
 return event.AsMouseEvent()->x() < kLeftPart;
}
