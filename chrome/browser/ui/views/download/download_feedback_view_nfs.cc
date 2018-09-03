// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/download/download_feedback_view_nfs.h"

#include <string>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "grit/components_strings.h"
#include "grit/generated_resources.h"
#include "grit/ui_resources_nfs.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/label.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/widget/widget.h"
#include "ui/aura/window.h"
#include "ui/views/border.h"
#include "ui/strings/grit/ui_strings.h"
#include "url/gurl.h"

using views::GridLayout;
using views::ColumnSet;
using views::Border;
using views::Label;

namespace {
const int kDownloadBubbleViewWidth = 240;
const int kDownloadBubbleViewMargin = 14;
const int kDownloadBubbleViewControlsPadding = 10;
const int kImageViewLeftMargin = 10;
const int kImageViewSize = 40;
const int kBubbleViewBottomInset = 40;
const int kBubbleViewVerticalMargin = 4;

//Auto close bubble in kAutoCloseDelay seconds  if no user action on bubble.
const int kAutoCloseDelay = 3;

const int kBackground_color = SkColorSetARGB(0xe0, 0x6a, 0x6a, 0x6a);

}  // namespace

DownloadFeedbackView* DownloadFeedbackView::download_bubble_ = NULL;

DownloadFeedbackView::DownloadFeedbackView(
    views::View* anchor_view,
    const DownloadInfo& info)
    : BubbleDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT, true),
      downloadInfo_(info),
      close_reason_(CLOSE_REASON_OTHER) {
    set_color((SkColor)kBackground_color);
    set_shadow(views::BubbleBorder::NO_SHADOW);
    set_margins(gfx::Insets(kBubbleViewVerticalMargin, 0, kBubbleViewVerticalMargin, 0));
    set_anchor_view_insets(gfx::Insets(kBubbleViewBottomInset, 0, 0, 0));

    StartAutoCloseTimer();
  }

DownloadFeedbackView::~DownloadFeedbackView() {
}

DownloadFeedbackView::DownloadInfo::DownloadInfo()
  : browser(nullptr),
    image(gfx::ImageSkia()),
    state(DOWNLOAD_STATE_NONE),
    file_name(base::string16()),
    file_size(base::string16()),
    item_id(0) {
}

DownloadFeedbackView::DownloadInfo::DownloadInfo(const DownloadInfo& info) = default;


DownloadFeedbackView::DownloadInfo::~DownloadInfo() {
}

void DownloadFeedbackView::Init() {
   // Column sets used in the layout of the bubble.
  enum ColumnSetID {
    CONTENT_COLUMN_SET_ID,
    BOTTOM_COLUMN_SET_ID,
  };

  GridLayout* layout = new GridLayout(this);
  SetLayoutManager(layout);

   // The column layout used for cotent rows.
  ColumnSet* cs = layout->AddColumnSet(CONTENT_COLUMN_SET_ID);
  cs->AddPaddingColumn(0, kDownloadBubbleViewControlsPadding);
  cs->AddColumn(GridLayout::LEADING, GridLayout::LEADING, 0,
          GridLayout::USE_PREF, 0, 0);
  cs->AddPaddingColumn(0, kDownloadBubbleViewMargin);
  cs->AddColumn(GridLayout::LEADING, GridLayout::LEADING, 0, GridLayout::FIXED,
                    kDownloadBubbleViewWidth - kImageViewLeftMargin -
                    kDownloadBubbleViewMargin - kImageViewSize -
                    kDownloadBubbleViewControlsPadding, 0);
  cs->AddPaddingColumn(0, kDownloadBubbleViewMargin);


  // The column layout used for bottom row.
  cs = layout->AddColumnSet(BOTTOM_COLUMN_SET_ID);
  cs->AddPaddingColumn(0, kDownloadBubbleViewMargin);
  cs->AddColumn(GridLayout::LEADING, GridLayout::CENTER, 0,
          GridLayout::USE_PREF, 0, 0);
  cs->AddPaddingColumn(0, kDownloadBubbleViewControlsPadding);
  cs->AddColumn(GridLayout::TRAILING, GridLayout::CENTER, 1,
          GridLayout::USE_PREF, 0, 0);
  cs->AddPaddingColumn(0, kDownloadBubbleViewControlsPadding);
  cs->AddColumn(GridLayout::LEADING, GridLayout::CENTER, 0,
          GridLayout::USE_PREF, 0, 0);
  cs->AddPaddingColumn(0, kDownloadBubbleViewMargin);

  //Add content rows
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  layout->StartRow(0, CONTENT_COLUMN_SET_ID);
  image_ = new views::ImageView();
  image_->SetImageSize(gfx::Size(kImageViewSize, kImageViewSize));
  image_->SetImage(downloadInfo_.image);
  layout->AddView(image_, 1, 4);
  status_label_ = new views::Label();
  if (downloadInfo_.state == DOWNLOAD_SUCCESS) {
    status_label_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_COMPLETE));
  } else {
    status_label_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_FAILED));
  }
  status_label_->SetEnabledColor(SK_ColorWHITE);
  status_label_->SetBackgroundColor((SkColor)kBackground_color);
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label_->SetElideBehavior(gfx::ELIDE_TAIL);
  layout->AddView(status_label_);

  layout->StartRowWithPadding(0, CONTENT_COLUMN_SET_ID, 0, 4);
  file_label_ = new views::Label();
  file_label_->SetText(downloadInfo_.file_name);
  file_label_->SetEnabledColor(SK_ColorWHITE);
  file_label_->SetBackgroundColor((SkColor)kBackground_color);
  file_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  file_label_->SetElideBehavior(gfx::ELIDE_TAIL);
  layout->SkipColumns(1);
  layout->AddView(file_label_);

  layout->StartRow(0, CONTENT_COLUMN_SET_ID);
  size_label_ = new views::Label();
  size_label_->SetText(downloadInfo_.file_size);
  size_label_->SetEnabledColor(SK_ColorWHITE);
  size_label_->SetBackgroundColor((SkColor)kBackground_color);
  size_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  size_label_->SetElideBehavior(gfx::ELIDE_TAIL);
  layout->SkipColumns(1);
  layout->AddView(size_label_);

  //Add bottom row
  layout->StartRow(0, BOTTOM_COLUMN_SET_ID);
  checkbox_view_ = new views::Checkbox(
        l10n_util::GetStringUTF16(IDS_CLOSE_MULTI_TABS_NO_REMAINDER));

  // Unchecked/Unfocused images.
  checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_NORMAL,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_N));
  checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_HOVERED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_H));
  checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_PRESSED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_N));
  checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_DISABLED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_N));

  // Checked/Unfocused images.
  checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_NORMAL,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));
  checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_HOVERED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));
  checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_PRESSED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));
  checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_DISABLED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));

  // Unchecked/Focused images.
  checkbox_view_->SetCustomImage(false, true, views::Checkbox::STATE_NORMAL,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_N));
  checkbox_view_->SetCustomImage(false, true, views::Checkbox::STATE_HOVERED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_H));
  checkbox_view_->SetCustomImage(false, true, views::Checkbox::STATE_PRESSED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_N));

  // Checked/Focused images.
  checkbox_view_->SetCustomImage(true, true, views::Checkbox::STATE_NORMAL,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));
  checkbox_view_->SetCustomImage(true, true, views::Checkbox::STATE_HOVERED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));
  checkbox_view_->SetCustomImage(true, true, views::Checkbox::STATE_PRESSED,
                 *rb.GetImageSkiaNamed(IDD_DOWNLOAD_CHECKBOX_P));

  checkbox_view_->SetEnabledTextColors(SK_ColorWHITE);
  checkbox_view_->SetLabelBackgroundColor((SkColor)kBackground_color);
  layout->AddView(checkbox_view_);
  if (downloadInfo_.state == DOWNLOAD_SUCCESS) {
    left_link_ = new views::Link(l10n_util::GetStringUTF16(IDS_DOWNLOAD_LINK_OPEN_DOWNLOAD));
    right_link_ = new views::Link(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_FOLDERS));
  } else {
    left_link_ = new views::Link(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_LABEL_RETRY));
    right_link_ = new views::Link(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_LABEL_DELETE));
  }
  left_link_->SetEnabledColor(SK_ColorWHITE);
  left_link_->SetBackgroundColor((SkColor)kBackground_color);
  left_link_->set_listener(this);
  right_link_->SetEnabledColor(SK_ColorWHITE);
  right_link_->SetBackgroundColor((SkColor)kBackground_color);
  right_link_->set_listener(this);
  layout->AddView(left_link_);
  layout->AddView(right_link_);
}

// static
views::Widget* DownloadFeedbackView::ShowBubble(views::View* anchor_view,
                                    const gfx::Rect& anchor_rect,
                                    gfx::NativeView parent_window,
                                    const DownloadInfo& info) {
  if (download_bubble_) {
    download_bubble_->downloadInfo_ = info;
    download_bubble_->Reset();
    return download_bubble_->GetWidget();
  }

  download_bubble_ =
        new DownloadFeedbackView(anchor_view, info);
  if (!anchor_view) {
    download_bubble_->SetAnchorRect(anchor_rect);
    download_bubble_->set_parent_window(parent_window);
  }
  views::BubbleDelegateView::CreateBubble(download_bubble_)->Show();
  download_bubble_->SetArrowPaintType(views::BubbleBorder::PAINT_NONE);

  return download_bubble_->GetWidget();
}

void DownloadFeedbackView::Reset() {
  StartAutoCloseTimer();
  image_->SetImage(downloadInfo_.image);
  if (downloadInfo_.state == DOWNLOAD_SUCCESS) {
    status_label_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_COMPLETE));
    left_link_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_LINK_OPEN_DOWNLOAD));
    right_link_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_FOLDERS));
  } else {
    status_label_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_FAILED));
    left_link_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_LABEL_RETRY));
    right_link_->SetText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_FEEDBACK_LABEL_DELETE));
  }
  file_label_->SetText(downloadInfo_.file_name);
  size_label_->SetText(downloadInfo_.file_size);
  Layout();
  SchedulePaint();
}

void DownloadFeedbackView::Hide() {
  if (download_bubble_)
    download_bubble_->GetWidget()->Close();
}

void DownloadFeedbackView::WindowClosing() {
  // We have to reset |bubble_| here, not in our destructor, because we'll be
  // destroyed asynchronously and the shown state will be checked before then.
  DCHECK_EQ(download_bubble_, this);
  auto_close_timer_.Stop();
  if (!downloadInfo_.close_cb.is_null()) {
    downloadInfo_.close_cb.Run(close_reason_, checkbox_view_->checked(), downloadInfo_.item_id);
  }
  download_bubble_ = NULL;
}

bool DownloadFeedbackView::ShouldShowCloseButton() const {
  return true;
}

bool DownloadFeedbackView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  return BubbleDelegateView::AcceleratorPressed(accelerator);
}

bool  DownloadFeedbackView::CanActivate() const {
  return false;
}

bool DownloadFeedbackView::WantsMouseEventsWhenInactive() const {
  return true;
}

gfx::Size DownloadFeedbackView::GetPreferredSize() const {
  return gfx::Size(kDownloadBubbleViewWidth,
      views::BubbleDelegateView::GetPreferredSize().height());
}

void DownloadFeedbackView::OnMouseEntered(const ui::MouseEvent& event) {
  auto_close_timer_.Stop();
}

void DownloadFeedbackView::OnMouseExited(const ui::MouseEvent& event) {
  StartAutoCloseTimer();
}

void DownloadFeedbackView::LinkClicked(views::Link* source, int event_flags) {
  if (source == left_link_) {
    if (downloadInfo_.state == DOWNLOAD_SUCCESS) {
      close_reason_ = CLOSE_REASON_OPEN;
    } else {
      close_reason_ = CLOSE_REASON_RETRY;
    }
  } else if (source == right_link_) {
    if (downloadInfo_.state == DOWNLOAD_SUCCESS) {
      close_reason_ = CLOSE_REASON_OPEN_FOLDERS;
    } else {
      close_reason_ = CLOSE_REASON_DELETE;
    }
  }

  Hide();
}

const char* DownloadFeedbackView::GetClassName() const {
  return "DownloadFeedbackView";
}

views::View* DownloadFeedbackView::GetInitiallyFocusedView() {
  return nullptr;
}

void DownloadFeedbackView::AutoCloseBubble() {
  Hide();
}

void DownloadFeedbackView::StartAutoCloseTimer() {
  auto_close_timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(kAutoCloseDelay),
            this, &DownloadFeedbackView::AutoCloseBubble);
}
