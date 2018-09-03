// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_url_floating_view.h"

#include <algorithm>
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"
#include "chrome/browser/ui/views/event_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "grit/theme_resources.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/path.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "chrome/grit/generated_resources.h"
#include "grit/components_strings.h"
#include "ui/views/controls/button/blue_button.h"

using views::LabelButton;
using views::LabelButtonBorder;

namespace {
  const int kSlideDuration = 200;
  const gfx::ElideBehavior kElideBehavior = gfx::FADE_TAIL;
  const SkColor kUrlFloatingViewBorderColor = SkColorSetRGB(0x4f, 0xa7, 0xff);
  const SkColor kUrlFloatingViewBgColor = SkColorSetRGB(0xde, 0xec, 0xff);
  const SkColor kButtonHoverColor = SkColorSetRGB(0xe8, 0xf2, 0xff);

  class FloatingButton : public views::LabelButton {
   public:
    FloatingButton(views::ButtonListener* listener,
                       const base::string16& title)
        : LabelButton(listener, title) {
      SetElideBehavior(kElideBehavior);
      SetInkDropMode(InkDropMode::ON);
      set_has_ink_drop_action_on_click(true);
      show_animation_.reset(new gfx::SlideAnimation(this));
      show_animation_->Show();
    }

    std::unique_ptr<LabelButtonBorder> CreateDefaultBorder() const override {
      std::unique_ptr<LabelButtonBorder> border =
          LabelButton::CreateDefaultBorder();
       border->set_insets(gfx::Insets(1, 4, 1, 4));
      return border;
    }

    bool IsTriggerableEvent(const ui::Event& e) override {
      return e.type() == ui::ET_GESTURE_TAP ||
             e.type() == ui::ET_GESTURE_TAP_DOWN ||
             event_utils::IsPossibleDispositionEvent(e);
    }

    std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override {
      return base::MakeUnique<views::FloodFillInkDropRipple>(
        gfx::Rect(gfx::Size(0, 0)), GetInkDropCenterBasedOnLastEvent(),
        color_utils::DeriveDefaultIconColor(kButtonHoverColor),
        ink_drop_visible_opacity());
    }

    std::unique_ptr<views::InkDropHighlight>
        CreateInkDropHighlight() const override {
      gfx::Size size = GetPreferredSize();
      return base::MakeUnique<views::InkDropHighlight>(
        size, kInkDropSmallCornerRadius,
        gfx::RectF(gfx::SizeF(size)).CenterPoint(),
        color_utils::DeriveDefaultIconColor(kButtonHoverColor));
    }

   private:
    std::unique_ptr<gfx::SlideAnimation> show_animation_;

    // Controls the visual feedback for the button state.
    // views::ButtonInkDropDelegate ink_drop_delegate_;

    DISALLOW_COPY_AND_ASSIGN(FloatingButton);
  };

} //namespage

class OmniboxUrlFloatingView::PopupWidget
    : public ThemeCopyingWidget,
      public base::SupportsWeakPtr<PopupWidget> {
 public:
  explicit PopupWidget(views::Widget* role_model)
      : ThemeCopyingWidget(role_model) {}
  ~PopupWidget() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PopupWidget);
};

////////////////////////////////////////////////////////////////////////////////
// OmniboxUrlFloatingView, public:
OmniboxUrlFloatingView* OmniboxUrlFloatingView::Create(
                const OmniboxViewViews* omnibox_view,
                const base::Callback<void(int)>& close_callback) {
  OmniboxUrlFloatingView* view = nullptr;
  view = new OmniboxUrlFloatingView(omnibox_view, close_callback);
  return view;
}

OmniboxUrlFloatingView::OmniboxUrlFloatingView(
                const OmniboxViewViews* omnibox_view,
                const base::Callback<void(int)>& close_callback)
    : omnibox_view_(omnibox_view),
      size_animation_(this),
      copy_button_(new FloatingButton(this, l10n_util::GetStringUTF16(IDS_URL_COPY))),
      paste_button_(new FloatingButton(this, l10n_util::GetStringUTF16(IDS_URL_PASTE_AND_OPEN))),
      separator_(new views::Separator(views::Separator::VERTICAL)),
      callback_(close_callback) {
  // The contents is owned by the LocationBarView.
  set_owned_by_client();

  separator_->SetColor(kUrlFloatingViewBorderColor);
  SetBorder(views::Border::CreateSolidBorder(1, kUrlFloatingViewBorderColor));
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kHorizontal, 0, 0, 0));

  size_animation_.SetSlideDuration(kSlideDuration);

  // registrar_.Add(this, chrome::NOTIFICATION_MOUSE_PRESSED,
  //                content::NotificationService::AllSources());
}

OmniboxUrlFloatingView::~OmniboxUrlFloatingView() {
  // registrar_.Remove(this, chrome::NOTIFICATION_MOUSE_PRESSED,
  //          content::NotificationService::AllSources());
}

gfx::Rect OmniboxUrlFloatingView::GetPopupBounds() const {
  if (!size_animation_.is_animating())
    return target_bounds_;

  gfx::Rect current_frame_bounds = start_bounds_;
  int current_height = static_cast<int>(
      size_animation_.GetCurrentValue() * target_bounds_.height());
  current_frame_bounds.set_y(start_bounds_.y() - current_height);
  current_frame_bounds.set_height(current_height);
  return current_frame_bounds;
}

void OmniboxUrlFloatingView::Show(const gfx::Point& location, int option) {
  Init(location, option);

  if (popup_ == NULL) {
    views::Widget* popup_parent = const_cast<views::Widget* >(omnibox_view_->GetWidget());

    // If the popup is currently closed, we need to create it.
    popup_ = (new PopupWidget(popup_parent))->AsWeakPtr();

    views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
#if defined(OS_WIN)
    // On Windows use the software compositor to ensure that we don't block
    // the UI thread blocking issue during command buffer creation. We can
    // revert this change once http://crbug.com/125248 is fixed.
    params.force_software_compositing = true;
#endif
    params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
    params.parent = popup_parent->GetNativeView();
    gfx::Rect initial_bounds = start_bounds_;
    params.bounds = initial_bounds;
    params.context = popup_parent->GetNativeWindow();
    popup_->Init(params);
    // Third-party software such as DigitalPersona identity verification can
    // hook the underlying window creation methods and use SendMessage to
    // synchronously change focus/activation, resulting in the popup being
    // destroyed by the time control returns here.  Bail out in this case to
    // avoid a NULL dereference.
    if (!popup_.get())
      return;
    popup_->SetVisibilityAnimationTransition(views::Widget::ANIMATE_NONE);
    popup_->SetContentsView(this);
    popup_->StackAtTop();
    popup_->ShowInactive();
  }

 size_animation_.Show();
 // popup_->SetBounds(GetPopupBounds());
}

////////////////////////////////////////////////////////////////////////////////
//OmniboxUrlFloatingView, OmniboxUrlFloatingView overrides:
bool OmniboxUrlFloatingView::IsOpen() const {
  return popup_ != NULL;
}

void  OmniboxUrlFloatingView::ClosePopup() {
    if (popup_ != NULL) {
      size_animation_.Stop();
      lastPrefSize_ = gfx::Size();
      popup_->Close();
      popup_.reset();
    }
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxUrlFloatingView, AnimationDelegate implementation:
void OmniboxUrlFloatingView::AnimationProgressed(
    const gfx::Animation* animation) {
  // We should only be running the animation when the popup is already visible.
  DCHECK(popup_ != NULL);
  popup_->SetBounds(GetPopupBounds());
}

 void OmniboxUrlFloatingView::ButtonPressed(views::Button* sender, const ui::Event& event) {
    if (!callback_.is_null()) {
      if (sender == copy_button_ ) {
        callback_.Run(static_cast<int>(URL_COPY));
      } else if (sender == paste_button_) {
        callback_.Run(static_cast<int>(URL_PASTE_AND_GO));
      }
    }
    ClosePopup();
 }

//  void OmniboxUrlFloatingView::Observe(int type,
//                     const content::NotificationSource& source,
//                     const content::NotificationDetails& details) {
//   switch (type) {
//     case chrome::NOTIFICATION_MOUSE_PRESSED:
//       ClosePopup();
//       break;
//     default:
//       NOTREACHED() << "Received unexpected notification " << type;
//   }
// }

////////////////////////////////////////////////////////////////////////////////
// OmniboxUrlFloatingView, private:
void OmniboxUrlFloatingView::UpdateBounds() {
  gfx::Size size = GetPreferredSize();
  if (size != lastPrefSize_) {
    start_bounds_.set_x(location_.x());
    start_bounds_.set_y(location_.y() + size.height());
    start_bounds_.set_width(size.width());
    start_bounds_.set_height(0);
    target_bounds_ = start_bounds_;
    target_bounds_.set_y(location_.y());
    target_bounds_.set_height(size.height());
  }

  lastPrefSize_ = size;
}

void OmniboxUrlFloatingView::Init(const gfx::Point& location, int option) {
  location_ = location;
  if (child_count()) {
    RemoveAllChildViews(false);
  }

  if (option & URL_COPY) {
    AddChildView(copy_button_);
  }
  if (option & URL_COPY && option & URL_PASTE_AND_GO) {
    AddChildView(separator_);
  }
  if (option & URL_PASTE_AND_GO) {
    AddChildView(paste_button_);
  }

  UpdateBounds();
  size_animation_.Reset();
}

// views::View overrides:
const char* OmniboxUrlFloatingView::GetClassName() const {
  return "OmniboxUrlFloatingView";
}

void OmniboxUrlFloatingView::OnPaintBackground(gfx::Canvas* canvas) {
  canvas->FillRect(gfx::Rect(size()), kUrlFloatingViewBgColor);
}

void OmniboxUrlFloatingView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  UpdateBounds();
}
