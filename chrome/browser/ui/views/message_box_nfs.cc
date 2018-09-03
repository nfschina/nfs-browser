// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/message_box_nfs.h"

#include "base/compiler_specific.h"
#include "ui/base/default_style.h"
#include "ui/base/hit_test.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/platform_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "grit/components_strings.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/views/shadow_view.h"

namespace nfsbrowser {

const int kMessagebox_max_width = 400;

class FixedWidthMessageBoxView : public views::MessageBoxView {
  public:
    FixedWidthMessageBoxView(views::MessageBoxView::InitParams params);
    ~FixedWidthMessageBoxView() override;

  // Overridden from View:
  gfx::Size GetPreferredSize() const override;
};

FixedWidthMessageBoxView::FixedWidthMessageBoxView(
          views::MessageBoxView::InitParams params)
    : views::MessageBoxView(params) {
}

FixedWidthMessageBoxView::~FixedWidthMessageBoxView() {
}

gfx::Size FixedWidthMessageBoxView::GetPreferredSize() const  {
  return gfx::Size(kMessagebox_max_width,
                         views::MessageBoxView::GetPreferredSize().height());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// SimpleMessageBoxViews, public:
SimpleMessageBoxViews::InitParams::InitParams(gfx::NativeWindow parent,
                                            MessageBoxType type,
                                            const base::string16& title,
                                            const base::string16& message)
        : parent(parent),
          type(type),
          title(title),
          message(message),
          position(gfx::Point(-1, -1)) {
}

SimpleMessageBoxViews::InitParams::~InitParams() {
}

SimpleMessageBoxViews::SimpleMessageBoxViews(const InitParams& param)
      : parent_(param.parent),
        window_title_(param.title),
        type_(param.type),
        yes_text_(param.yes_text),
        no_text_(param.no_text),
        is_system_modal_(param.parent == NULL),
        contents_view_(new views::TopContentsView(new FixedWidthMessageBoxView(
                                  views::MessageBoxView::InitParams(param.message)), this)),
        checkbox_view_(nullptr),
        callback_(param.callback),
        position_(param.position) {
    if (yes_text_.empty()) {
      yes_text_ =
          type_ == MESSAGE_BOX_TYPE_QUESTION
              ? l10n_util::GetStringUTF16(IDS_CONFIRM_MESSAGEBOX_YES_BUTTON_LABEL)
              : l10n_util::GetStringUTF16(IDS_OK);
  }

  if (no_text_.empty() && type_ == MESSAGE_BOX_TYPE_QUESTION)
    no_text_ = l10n_util::GetStringUTF16(IDS_CANCEL);

  if (!param.checkbox_text.empty() && type_ == MESSAGE_BOX_TYPE_QUESTION) {
    checkbox_view_ = new views::Checkbox(param.checkbox_text);

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
     // Unchecked/Unfocused images.
    checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_NORMAL,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_N));
    checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_HOVERED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_H));
    checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_PRESSED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_P));
    checkbox_view_->SetCustomImage(false, false, views::Checkbox::STATE_DISABLED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_N));

    // Checked/Unfocused images.
    checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_NORMAL,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_N));
    checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_HOVERED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_H));
    checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_PRESSED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_P));
    checkbox_view_->SetCustomImage(true, false, views::Checkbox::STATE_DISABLED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_N));

    // Unchecked/Focused images.
    checkbox_view_->SetCustomImage(false, true, views::Checkbox::STATE_NORMAL,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_N));
    checkbox_view_->SetCustomImage(false, true, views::Checkbox::STATE_HOVERED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_H));
    checkbox_view_->SetCustomImage(false, true, views::Checkbox::STATE_PRESSED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_UNCHECK_P));

    // Checked/Focused images.
    checkbox_view_->SetCustomImage(true, true, views::Checkbox::STATE_NORMAL,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_N));
    checkbox_view_->SetCustomImage(true, true, views::Checkbox::STATE_HOVERED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_H));
    checkbox_view_->SetCustomImage(true, true, views::Checkbox::STATE_PRESSED,
                   *rb.GetImageSkiaNamed(IDD_DIALOG_CHECKBOX_CHECKED_P));
  }
}

SimpleMessageBoxViews::~SimpleMessageBoxViews() {
}

int SimpleMessageBoxViews::GetDialogButtons() const {
  if (type_ == MESSAGE_BOX_TYPE_QUESTION)
    return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;

  return ui::DIALOG_BUTTON_OK;
}

base::string16 SimpleMessageBoxViews::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_CANCEL)
    return no_text_;
  return yes_text_;
}

bool SimpleMessageBoxViews::Cancel() {
  Done(MESSAGE_BOX_RESULT_NO);
  return true;
}

bool SimpleMessageBoxViews::Accept() {
  Done(MESSAGE_BOX_RESULT_YES);
  return true;
}

views::View* SimpleMessageBoxViews::CreateExtraView() {
  return checkbox_view_;
}

int SimpleMessageBoxViews::NonClientHitTest(const gfx::Point& point) {
  return contents_view_->NonClientHitTest(point);
}

base::string16 SimpleMessageBoxViews::GetWindowTitle() const {
  return window_title_;
}

void SimpleMessageBoxViews::DeleteDelegate() {
  delete this;
}

ui::ModalType SimpleMessageBoxViews::GetModalType() const {
  return is_system_modal_ ? ui::MODAL_TYPE_SYSTEM : ui::MODAL_TYPE_WINDOW;
}

views::View* SimpleMessageBoxViews::GetContentsView() {
  return contents_view_;
}

views::Widget* SimpleMessageBoxViews::GetWidget() {
  return contents_view_->GetWidget();
}

const views::Widget* SimpleMessageBoxViews::GetWidget() const {
  return contents_view_->GetWidget();
}

void SimpleMessageBoxViews::Show() {
  constrained_window::CreateNfsModalDialogViews(this, parent_, position_)->Show();
}

void SimpleMessageBoxViews::ShowNfsMessageBox(const InitParams& param) {
  if (!base::MessageLoopForUI::IsCurrent() ||
      !ResourceBundle::HasSharedInstance()) {
    LOG(ERROR) << "Unable to show a dialog outside the UI thread message loop: "
               << param.title << " - " << param.message;
    return;
  }

  SimpleMessageBoxViews* dialog = new SimpleMessageBoxViews(param);
  dialog->Show();
}

////////////////////////////////////////////////////////////////////////////////
// SimpleMessageBoxViews, private:
void SimpleMessageBoxViews::Done(MessageBoxResult result) {
  if (!callback_.is_null()) {
   if (checkbox_view_) {
     callback_.Run (result, checkbox_view_->checked());
    } else {
     callback_.Run (result, false);
    }
  }
}

}  // namespace nfsbrowser
