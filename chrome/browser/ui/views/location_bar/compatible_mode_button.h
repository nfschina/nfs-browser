// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_COMPATIBLE_MODE_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_COMPATIBLE_MODE_BUTTON_H_

#include "base/macros.h"
#include "ui/views/controls/button/image_button.h"

class Browser;

namespace content {
class WebContents;
}

class CompatibleModeButton : public views::ImageButton {
 public:
  CompatibleModeButton(Browser* browser);
  ~CompatibleModeButton() override;

  void Init();

  void UpdateImageForContents(content::WebContents* contents);

 protected:
  // views::ImageView:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;

  void UpdateImage();

 private:
  Browser* browser_;

  bool mode_state_;

  DISALLOW_COPY_AND_ASSIGN(CompatibleModeButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_COMPATIBLE_MODE_BUTTON_H_
