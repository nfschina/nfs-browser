// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_QR_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_QR_BUTTON_H_

#include "ui/views/controls/button/image_button.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

class Browser;

class QrButton : public views::ImageButton {
 public:
  QrButton(Browser* browser, LocationBarView::Delegate* location_bar_delegate);
  ~QrButton() override; 

  void Init();
  void SetMouseReleasedAction(bool action);
  void SetNextAction(bool action);
  bool UrlIsHttp();

 protected:
  // views::ImageView:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  GURL GetUrl();

 private:
  Browser* browser_;
  bool suppress_mouse_released_action_;
  bool next_mouse_;
  LocationBarView::Delegate* location_bar_delegate_;

  DISALLOW_COPY_AND_ASSIGN(QrButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_QR_BUTTON _VIEW_H_
