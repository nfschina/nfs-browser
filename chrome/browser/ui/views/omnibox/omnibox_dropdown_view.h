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

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_DROPDOWN_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_DROPDOWN_VIEW_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/omnibox/browser/omnibox_dropdown_model.h"
#include "components/omnibox/browser/omnibox_popup_view.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/view.h"
#include "ui/views/view_targeter_delegate.h"

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class LocationBarView;
class OmniboxEditModel;
class OmniboxDropdownEntryView;
class OmniboxView;
class Profile;
class Browser;

// A view representing the contents of the autocomplete popup.
class OmniboxDropdownView : public views::View,
                                 public OmniboxPopupView,
                                 public views::ViewTargeterDelegate,
                                 public OmniboxDropdownDataObserver,
                                 public gfx::AnimationDelegate,
                                 public content::NotificationObserver {
 public:
  // Factory method for creating the PopupView.
  static OmniboxDropdownView* Create(
                                  const gfx::FontList& font_list,
                                  OmniboxView* omnibox_view,
                                  OmniboxEditModel* edit_model,
                                  LocationBarView* location_bar_view);

  // Returns the bounds the popup should be shown at. This is the display bounds
  // and includes offsets for the dropshadow which this view's border renders.
  gfx::Rect GetPopupBounds() const;

  virtual void LayoutChildren();

  // OmniboxPopupView:
  bool IsOpen() const override;
  void InvalidateLine(size_t line) override;
  void OnLineSelected(size_t line) override;
  void UpdatePopupAppearance() override;
  gfx::Rect GetTargetBounds() override;
  void PaintUpdatesNow() override;
  void OnDragCanceled() override;
  void ClosePopup() override;

   // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;

  // views::View:
  void Layout() override;
  views::View* GetTooltipHandlerForPoint(const gfx::Point& point) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  bool IsSelectedIndex(size_t index) const;
  bool IsHoveredIndex(size_t index) const;
  void RemoveEntry(size_t line);
  size_t GetIndexByUrl(const GURL& url);

  int max_match_contents_width() const { return max_match_contents_width_; }
  int start_margin() const { return start_margin_; }
  int end_margin() const { return end_margin_; }


  // Find the index of the match under the given |point|, specified in window
  // coordinates. Returns OmniboxPopupModel::kNoMatch if there isn't a match at
  // the specified point.
  size_t GetIndexForPoint(const gfx::Point& point);

 protected:
  OmniboxDropdownView(
                           const gfx::FontList& font_list,
                           OmniboxView* omnibox_view,
                           OmniboxEditModel* edit_model,
                           LocationBarView* location_bar_view);
  ~OmniboxDropdownView() override;

  LocationBarView* location_bar_view() { return location_bar_view_; }

  // Calculates the height needed to show all the results in the model.
  int CalculatePopupHeight();
  OmniboxDropdownEntryView* CreateItemView(
      const gfx::FontList& font_list);

 private:
  class PopupWidget;

  // views::View:
  const char* GetClassName() const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void PaintChildren(const ui::PaintContext& context) override;

  // views::ViewTargeterDelegate:
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;

  //OmniboxDropdownDataObserver:
  void OnOmniboxDropdownDataLoaded() override;
  void OnOmniboxDropdownDataChanged() override;
  void OnOmniboxDropdownEntryUpdate(
      const std::vector<GURL>& urls) override;

  void AddChildViews();

  // Call immediately after construction.
  //void Init();

   // Processes a located event (e.g. mouse/gesture) and sets the selection/hover
  // state of a line in the list.
  void UpdateLineEvent(const ui::LocatedEvent& event,
                       bool should_set_selected_line);

  // Opens an entry from the list depending on the event and the selected
  // disposition.
  void OpenSelectedLine(const ui::LocatedEvent& event,
                        WindowOpenDisposition disposition);

  OmniboxDropdownEntryView* Item_view_at(size_t i);
  OmniboxDropdownEntryView* GetItemByUrl(const GURL& url);

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void Invalidate();

  size_t EntryCountToShow();

  void UpdateEntry(const GURL& url);

  std::unique_ptr<OmniboxDropdownModel> model_;

  // The popup that contains this view.  We create this, but it deletes itself
  // when its window is destroyed.  This is a WeakPtr because it's possible for
  // the OS to destroy the window and thus delete this object before we're
  // deleted, or without our knowledge.
  base::WeakPtr<PopupWidget> popup_;

  // The edit view that invokes us.
  OmniboxView* omnibox_view_;

  LocationBarView* location_bar_view_;

  // The font list used for result rows, based on the omnibox font list.
  gfx::FontList font_list_;

  // If the user cancels a dragging action (i.e. by pressing ESC), we don't have
  // a convenient way to release mouse capture. Instead we use this flag to
  // simply ignore all remaining drag events, and the eventual mouse release
  // event. Since OnDragCanceled() can be called when we're not dragging, this
  // flag is reset to false on a mouse pressed event, to make sure we don't
  // erroneously ignore the next drag.
  bool ignore_mouse_drag_;

  gfx::Rect start_bounds_;
  gfx::Rect target_bounds_;

  int start_margin_;
  int end_margin_;

  // These pointers are owned by the resource bundle.
  const gfx::ImageSkia* top_shadow_ = nullptr;
  const gfx::ImageSkia* bottom_shadow_ = nullptr;

  // When the dropdown is not wide enough while displaying postfix suggestions,
  // we use the width of widest match contents to shift the suggestions so that
  // the widest suggestion just reaches the end edge.
  int max_match_contents_width_;

  // The popup sizes vertically using an animation when the popup is getting
  // shorter (not larger, that makes it look "slow").
  gfx::SlideAnimation size_animation_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxDropdownView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_POPUP_CONTENTS_VIEW_H_
