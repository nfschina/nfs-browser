// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/qr_button.h"

#include "chrome/browser/ui/view_ids.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/browser/ui/browser_commands.h"
#include "grit/components_strings.h"
#include "grit/theme_resources.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/resource/resource_bundle.h"
#include "third_party/QR/src/qrenc.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "components/toolbar/toolbar_model.h"

using base::Time;
using base::TimeDelta;

QrButton::QrButton(Browser* browser, LocationBarView::Delegate* location_bar_delegate)
  :views::ImageButton(NULL),
  browser_(browser),
  suppress_mouse_released_action_(false),
  next_mouse_(false),
  location_bar_delegate_(location_bar_delegate) {
  set_id(VIEW_ID_QR_BUTTON);
  Init();
}

QrButton::~QrButton() {
}

void QrButton::Init() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  SetImage(views::CustomButton::STATE_NORMAL, rb.GetImageSkiaNamed(IDD_QR_N));
  SetImage(views::CustomButton::STATE_HOVERED, rb.GetImageSkiaNamed(IDD_QR_H));
  views::ImageButton::SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_ACROSS_SCREEN));//yana add 160729
}

bool QrButton::OnMousePressed(const ui::MouseEvent& event) {
  if(state() == STATE_DISABLED)
    return false;

  next_mouse_ = !next_mouse_;
  return true;
}

void QrButton::OnMouseReleased(const ui::MouseEvent& event) {
  if(state() == STATE_DISABLED) {
    return;
  }

  if(!next_mouse_) 
    return;

  if(!suppress_mouse_released_action_) {
    chrome::QrCurrentPage(browser_);
    suppress_mouse_released_action_ = true;
  }
}

GURL QrButton::GetUrl() {
  GURL url = location_bar_delegate_->GetToolbarModel()->GetURL();
  return url;
}

void QrButton::OnMouseEntered(const ui::MouseEvent& event) {
  bool is_http = UrlIsHttp();
  if(is_http) {
    SetState(STATE_HOVERED);
  }else
    SetState(STATE_DISABLED);

}

bool QrButton::UrlIsHttp() {
  GURL url = GetUrl();
  if(url.is_empty())
    return false;

  if(!url.spec().compare(0, 4, "http"))
    return true;
  else if(!url.spec().compare(0, 5, "https"))
    return true;

  return false;
}

void QrButton::SetMouseReleasedAction(bool action) {
  suppress_mouse_released_action_ = action;
}

void QrButton::SetNextAction(bool action) {
  next_mouse_ = action;
}

