// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_DROP_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_DROP_VIEW_H_

#include "base/macros.h"
//#include "ui/views/controls/image_view.h"
#include "ui/views/controls/button/image_button.h"

class Browser;

class DropDownView : public views::ImageButton {
 public:
  DropDownView(Browser* browser);
  ~DropDownView() override;

  void Init();

 protected:
  // views::ImageView:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;

 private:
  Browser* browser_;

  bool suppress_mouse_released_action_;

  DISALLOW_COPY_AND_ASSIGN(DropDownView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_DROP_VIEW_H_
