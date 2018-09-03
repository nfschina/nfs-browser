// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/drop_down_view.h"

#include "base/metrics/histogram.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/grit/generated_resources.h"
#include "grit/components_strings.h"
#include "grit/theme_resources.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/vector_icons_public.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

DropDownView::DropDownView(Browser* browser)
  :views::ImageButton(NULL),
  browser_(browser),
  suppress_mouse_released_action_(false) {
  set_id(VIEW_ID_DROPDOWN_BUTTON);
  Init();
}

DropDownView::~DropDownView() {
}

void DropDownView::Init() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  SetImage(views::CustomButton::STATE_NORMAL, rb.GetImageSkiaNamed(IDD_DROPDOWN_N));
  SetImage(views::CustomButton::STATE_HOVERED, rb.GetImageSkiaNamed(IDD_DROPDOWN_H));
  views::ImageButton::SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_DROPDOWN_LIST));//yana add 160727
  //SetImage(views::CustomButton::STATE_PRESSED, rb.GetImageSkiaNamed(IDD_DROPDOWN_H));
}

bool DropDownView::OnMousePressed(const ui::MouseEvent& event) {
  suppress_mouse_released_action_ = BrowserView::GetBrowserViewForBrowser(browser_)->
                  GetLocationBarView()->IsOmniboxDropdownViewOpen();

  return true;
}

void DropDownView::OnMouseReleased(const ui::MouseEvent& event) {
  if (suppress_mouse_released_action_) {
    suppress_mouse_released_action_ = false;
    BrowserView::GetBrowserViewForBrowser(browser_)->GetLocationBarView()->CloseOmniboxDropdownView();
    return;
  }
  BrowserView::GetBrowserViewForBrowser(browser_)->GetLocationBarView()->ShowOmniboxDropdownView();
}
