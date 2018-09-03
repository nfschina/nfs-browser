// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MANAGER_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MANAGER_BUTTON_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/views/controls/button/image_button.h"

class Browser;

class AppManagerButton : public views::ImageButton {
 public:
  AppManagerButton(Browser* browser,
             views::ButtonListener* listener);
  ~AppManagerButton() override;

 private:
  const char* GetClassName() const override;
  void OnMouseReleased(const ui::MouseEvent& event) override;

  //views::View overrides
  void ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) override;
  void OnThemeChanged() override;

  void SetDark(bool dark) { is_dark_ = dark; }

  bool IsDark() { return is_dark_; }

  void LoadImage();

  Browser* browser_;

  //is dark button
  bool is_dark_;

  base::WeakPtrFactory<AppManagerButton> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppManagerButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MANAGER_BUTTON_H_