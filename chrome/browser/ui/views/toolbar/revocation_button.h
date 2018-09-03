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

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_REVOCATION_BUTTON_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_REVOCATION_BUTTON_H_

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

class Browser;

class RevocationButton : public ToolbarButton {
 public:
  RevocationButton(Browser* browser,
                       views::ButtonListener* listener,
                       ui::MenuModel* model);
  ~RevocationButton() override;

 private:
  // ToolbarButton:
  const char* GetClassName() const override;
  // ToolbarButton:
  void NotifyClick(const ui::Event& event) override;
  //void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;

  void SetRealImage(bool left);
  bool IsLeftPart(const ui::MouseEvent& event);

  bool last_on_left_;
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(RevocationButton);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_REVOCATION_BUTTON_H_
