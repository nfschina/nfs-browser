// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_URL_FLOATING_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_URL_FLOATING_VIEW_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/view_targeter_delegate.h"

class OmniboxViewViews;

namespace views {
class LabelButton;
class Separator;
}

namespace gfx {
class Point;
}

class OmniboxUrlFloatingView : public views::View,
                                 public gfx::AnimationDelegate,
                                 public views::ButtonListener {
 public:
  enum Url_option {
    URL_NONE = 0,
    URL_COPY = 1 << 0,
    URL_PASTE_AND_GO = 1 << 1
  };

  // Factory method for creating the PopupView.
  static OmniboxUrlFloatingView* Create(const OmniboxViewViews* omnibox_view,
                                  const base::Callback<void(int)>& close_callback);

  ~OmniboxUrlFloatingView() override;

  void Show(const gfx::Point& location, int option);

  // Returns the bounds the popup should be shown at. This is the display bounds
  // and includes offsets for the dropshadow which this view's border renders.
  gfx::Rect GetPopupBounds() const;

  bool IsOpen() const;
  void ClosePopup();

   // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;

  //ButtonListener
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  //  // content::NotificationObserver overrides.
  // void Observe(int type,
  //              const content::NotificationSource& source,
  //              const content::NotificationDetails& details) override;


 protected:
  OmniboxUrlFloatingView(const OmniboxViewViews* omnibox_view,
                                  const base::Callback<void(int)>& close_callback);

 private:
  class PopupWidget;

  void UpdateBounds();
  void Init(const gfx::Point& location, int option);

  // views::View:
  const char* GetClassName() const override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // void OnPaint(gfx::Canvas* canvas) override;
  // void PaintChildren(const ui::PaintContext& context) override;

  // The popup that contains this view.  We create this, but it deletes itself
  // when its window is destroyed.  This is a WeakPtr because it's possible for
  // the OS to destroy the window and thus delete this object before we're
  // deleted, or without our knowledge.
  base::WeakPtr<PopupWidget> popup_;

  // The edit view that invokes us.
  const OmniboxViewViews* omnibox_view_;

  gfx::Rect start_bounds_;
  gfx::Rect target_bounds_;
  gfx::Point location_;
  gfx::Size lastPrefSize_;
  gfx::SlideAnimation size_animation_;
  views::LabelButton* copy_button_;
  views::LabelButton* paste_button_;
  views::Separator* separator_;
  base::Callback<void(int)> callback_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxUrlFloatingView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_URL_FLOATING_VIEW_H_
