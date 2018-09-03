// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SEARCH_BAR_SEARCH_ICON_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_SEARCH_BAR_SEARCH_ICON_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/button/image_button.h"
#include "chrome/browser/ui/search_bar/search_engine_menu_model.h"
class Profile;//yana add 160623

namespace views {
  class MenuRunner;
}
class SearchBarView;
class SearchEngineMenuModel;


class SearchIconView : public views::ImageButton,
                       public SearchEngineMenuModelDelegate {
 public:
  // The button's class name.
  static const char kViewClassName[];

  SearchIconView(Profile* profile,SearchBarView* searchbar, SearchEngineMenuModel* menu_model);
  ~SearchIconView() override;

  // Overridden from views::ImageView:
  // If menu is currently pending for long press - stop it.
  void ClearPendingMenu();

  // Indicates if menu is currently showing.
  bool IsMenuShowing() const;

  // Overridden from views::View
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  const char* GetClassName() const override;
  // Showing the drop down results in a MouseCaptureLost, we need to ignore it.
  void OnMouseCaptureLost() override {}
  void OnMouseExited(const ui::MouseEvent& event) override;
  // Display the right-click menu, as triggered by the keyboard, for instance.
  // Using the member function ShowDropDownMenu for the actual display.
  void ShowContextMenu(const gfx::Point& p,
      ui::MenuSourceType source_type) override;
  gfx::Size GetPreferredSize() const override;
  void OnPaint(gfx::Canvas* canvas) override;

  void GetAccessibleState(ui::AXViewState* state) override;

  // ui::EventHandler overrides:
  void OnGestureEvent(ui::GestureEvent* event) override;

  void OnDefaultSearchEngineChange() override;

  SearchBarView* GetSearchBarView() const { return searchbar_view_; }

  void ClearMenu();

protected:
  // Returns if menu should be shown. Override this to change default behavior.
  virtual bool ShouldShowMenu();

  // Function to show the dropdown menu.
  virtual void ShowDropDownMenu();

 private:
    Profile* profile_;//yana add 160623

  base::Time menu_closed_time_;

  // The model that populates the attached menu.
  //std::unique_ptr<ui::MenuModel> model_;
  ui::MenuModel* model_;//yana 160616

  // Indicates if menu is currently showing.
  bool menu_showing_;

  // Y position of mouse when left mouse button is pressed
  int y_position_on_lbuttondown_;

  // Menu runner to display drop down menu.
  std::unique_ptr<views::MenuRunner> menu_runner_;

  std::unique_ptr<views::ImageButton> arrow_button_;

  gfx::ImageSkia display_image_;

  SearchBarView* searchbar_view_;

  // A factory for tasks that show the dropdown context menu for the button.
  base::WeakPtrFactory<SearchIconView> show_menu_factory_;

  DISALLOW_COPY_AND_ASSIGN(SearchIconView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_SEARCH_BAR_SEARCH_ICON_VIEW_H_
