// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MESSAGE_BOX_NFS_H_
#define CHROME_BROWSER_UI_MESSAGE_BOX_NFS_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/widget/widget.h"

namespace nfsbrowser {

enum MessageBoxResult {
  MESSAGE_BOX_RESULT_NO = 0,  // User chose NO or CANCEL.
  MESSAGE_BOX_RESULT_YES = 1, // User chose YES or OK.
};

enum MessageBoxType {
  MESSAGE_BOX_TYPE_WARNING,      // Shows an OK button.
  MESSAGE_BOX_TYPE_QUESTION,     // Shows YES and NO buttons.
};

class SimpleMessageBoxViews : public views::NfsDialogDelegate {
 public:
  typedef base::Callback<void(MessageBoxResult, bool)> Close_CALLBACK;

  struct InitParams {
    explicit InitParams(gfx::NativeWindow parent,
                                            MessageBoxType type,
                                            const base::string16& title,
                                            const base::string16& message);
    ~InitParams();

    gfx::NativeWindow parent;
    MessageBoxType type;
    base::string16 title;
    //Content text of dialog.
    base::string16 message;
    base::string16 yes_text;
    base::string16 no_text;
    //If checkbox_text is empty, there'll be no checkbox.
    base::string16 checkbox_text;
    //Callback to run when user close dialog
    Close_CALLBACK callback;
    //Dialog position to show on(relative to browser window).
    gfx::Point position;
  };

  SimpleMessageBoxViews(const InitParams& param);
  ~SimpleMessageBoxViews() override;

  // Overridden from views::DialogDelegate:
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool Cancel() override;
  bool Accept() override;
  views::View* CreateExtraView() override;
  int NonClientHitTest(const gfx::Point& point) override;

  // Overridden from views::WidgetDelegate:
  base::string16 GetWindowTitle() const override;
  void DeleteDelegate() override;
  ui::ModalType GetModalType() const override;
  views::View* GetContentsView() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

  //Inner show MessageBox.
  void Show();

  //Wrapper function
  static void ShowNfsMessageBox(const InitParams& param);

 private:
  void Done(MessageBoxResult result);

  gfx::NativeWindow parent_;
  const base::string16 window_title_;
  const MessageBoxType type_;
  base::string16 yes_text_;
  base::string16 no_text_;
  bool is_system_modal_;
  views::TopContentsView* contents_view_;
  views::Checkbox* checkbox_view_;
  Close_CALLBACK callback_;
  gfx::Point position_;

  DISALLOW_COPY_AND_ASSIGN(SimpleMessageBoxViews);
};

}  // namespace nfsbrowser
#endif  // CHROME_BROWSER_UI_MESSAGE_BOX_nfs_H_
