// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SREACHBAR_SEARCH_BAR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_SREACHBAR_SEARCH_BAR_VIEW_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "components/prefs/pref_member.h"

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/search_bar/searchbox_edit_controller.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/painter.h"
#include "ui/views/view.h"
#include "ui/views/drag_controller.h"
#include "chrome/browser/ui/search_bar/search_engine_menu_model.h"//yana add 160612
namespace gfx {
class SlideAnimation;
}

namespace views {
class BubbleDelegateView;
class ImageButton;
class ImageView;
class Label;
class Widget;
}

class Browser;
class Profile;
class SearchboxViewViews;
class SearchIconView;

/////////////////////////////////////////////////////////////////////////////
//
// LocationBarView class
//
//   The LocationBarView class is a View subclass that paints the background
//   of the URL bar strip and contains its content.
//
/////////////////////////////////////////////////////////////////////////////
class SearchBarView : public views::View,
                      public views::ButtonListener,
                      public SearchboxEditController,
                      public views::DragController {
 public:
  // The location bar view's class name.
  static const char kViewClassName[];

  enum ColorKind {
    BACKGROUND = 0,
    TEXT,
    SELECTED_TEXT,
    DEEMPHASIZED_TEXT,
    SECURITY_TEXT,
  };

  SearchBarView(Browser* browser,
                Profile* profile,
                CommandUpdater* command_updater,
                LocationBarView::Delegate* delegate);

  ~SearchBarView() override;

  // Initializes the LocationBarView.
  void Init();

  SearchboxViewViews* searchbox_view() { return searchbox_view_views_; }
  const SearchboxViewViews* searchbox_view() const { return searchbox_view_views_; }

   // Returns the delegate.
  LocationBarView::Delegate* delegate() const { return delegate_; }

  CommandUpdater* command_updater() const { return command_updater_; }

  // Returns the screen coordinates of the search entry (where the URL text
  // appears, not where the icons are shown).
  gfx::Point GetSearchEntryOrigin() const;


  base::string16 GetInputString() const;
  WindowOpenDisposition GetWindowOpenDisposition() const;
  ui::PageTransition GetPageTransition() const;

  // views::View:
  bool HasFocus() const override;
  void GetAccessibleState(ui::AXViewState* state) override;
  gfx::Size GetPreferredSize() const override;
  void Layout() override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;
  void OnThemeChanged() override;

  //SearchboxEditController:
  void OnAutocompleteAccept(const GURL& url,
                                    WindowOpenDisposition disposition,
                                    ui::PageTransition transition) override;
  void OnInputInProgress(bool in_progress) override;
  void OnChanged() override;
  void OnSetFocus() override;
  void ShowURL() override;
  ToolbarModel* GetToolbarModel() override;
  const ToolbarModel* GetToolbarModel() const override;
  void OnKillFocus() override;

  int GetHorizontalEdgeThickness() const;
  int vertical_edge_thickness() const;

  void AcceptInput();
  content::WebContents* GetWebContents() const;
  bool ShouldOpenNewTabToNaviagate() const;

 private:
  std::unique_ptr<SearchEngineMenuModel> search_engine_menu_model_;//yana 0612
  // views::View:
  const char* GetClassName() const override;
  void OnFocus() override;
  void OnPaint(gfx::Canvas* canvas) override;

  void PaintChildren(const ui::PaintContext& context) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender,
                     const ui::Event& event) override;

  // views::DragController:
  void WriteDragDataForView(View* sender,
                                    const gfx::Point& press_pt,
                                    OSExchangeData* data) override;
  int GetDragOperationsForView(View* sender,
                                       const gfx::Point& p) override;
  bool CanStartDragForView(View* sender,
                                   const gfx::Point& press_pt,
                                   const gfx::Point& p) override;

  int GetInternalHeight(bool use_preferred_size);

  bool IsTextEmpty() const;

  // The Browser this LocationBarView is in.  Note that at least
  // chromeos::SimpleWebViewDialog uses a LocationBarView outside any browser
  // window, so this may be NULL.
  Browser* browser_;

   // The profile which corresponds to this View.
  Profile* profile_;

    // Command updater which corresponds to this View.
  CommandUpdater* command_updater_;

  // Our delegate.
  LocationBarView::Delegate* delegate_;

  // This is the string of text from the autocompletion session that the user
  // entered or selected.
  base::string16 search_input_;

    // The user's desired disposition for how their input should be opened
  WindowOpenDisposition disposition_;

  // The transition type to use for the navigation
  ui::PageTransition transition_;

  SearchIconView* search_icon_view_;

  views::ImageButton* search_button_;

    // the search box
  SearchboxViewViews* searchbox_view_views_;

  //[zhangyq]
  SkColor background_color_;

  // Object used to paint the border.
  std::unique_ptr<views::Painter> normal_background_;
  std::unique_ptr<views::Painter> hover_background_;

  DISALLOW_COPY_AND_ASSIGN(SearchBarView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_SREACHBAR_SEARCH_BAR_VIEW_H_
