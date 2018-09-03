// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/compatible_mode_button.h"

#include "base/metrics/histogram.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "grit/components_strings.h"
#include "grit/theme_resources.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/vector_icons_public.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

CompatibleModeButton::CompatibleModeButton(Browser* browser)
  :views::ImageButton(NULL),
  browser_(browser),
  mode_state_(false) {
  set_id(VIEW_ID_COMPATIBLE_MODE_BUTTON);
  Init();
}

CompatibleModeButton::~CompatibleModeButton() {
}

void CompatibleModeButton::Init() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  SetImage(views::CustomButton::STATE_NORMAL, rb.GetImageSkiaNamed(IDD_COMPATIBLE_MODE_OFF_N));
  SetImage(views::CustomButton::STATE_HOVERED, rb.GetImageSkiaNamed(IDD_COMPATIBLE_MODE_OFF_H));
  // SetImage(views::CustomButton::STATE_PRESSED, rb.GetImageSkiaNamed(IDD_DROPDOWN_H));
  views::ImageButton::SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_COMPATIBLE_MODE));
}

bool CompatibleModeButton::OnMousePressed(const ui::MouseEvent& event) {
  return true;
}

void CompatibleModeButton::OnMouseReleased(const ui::MouseEvent& event) {
  if (!mode_state_) {
    chrome::ExecuteCommand(browser_, IDC_COMPATIBLE_MODE_ON);
  } else {
    chrome::ExecuteCommand(browser_, IDC_COMPATIBLE_MODE_OFF);
  }
  mode_state_ = !mode_state_;
  UpdateImage();
}

void CompatibleModeButton::UpdateImageForContents(content::WebContents* contents) {
  if (contents) {
    content::RenderViewHost* host = contents->GetRenderViewHost();
    content::WebPreferences prefs = host->GetWebkitPreferences();
    mode_state_ = prefs.compatible_mode;
    UpdateImage();
  }
}

void CompatibleModeButton::UpdateImage() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (mode_state_) {
    SetImage(views::CustomButton::STATE_NORMAL, rb.GetImageSkiaNamed(IDD_COMPATIBLE_MODE_ON_N));
    SetImage(views::CustomButton::STATE_HOVERED, rb.GetImageSkiaNamed(IDD_COMPATIBLE_MODE_ON_H));
  } else {
    SetImage(views::CustomButton::STATE_NORMAL, rb.GetImageSkiaNamed(IDD_COMPATIBLE_MODE_OFF_N));
    SetImage(views::CustomButton::STATE_HOVERED, rb.GetImageSkiaNamed(IDD_COMPATIBLE_MODE_OFF_H));
  }
}
