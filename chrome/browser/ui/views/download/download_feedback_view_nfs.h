// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_FEEDBACK_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_FEEDBACK_VIEW_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/browser.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/window/non_client_view.h"

namespace bookmarks {
class BookmarkBubbleObserver;
}

namespace views {
class Label;
class Link;
class ImageView;
class Checkbox;
}

namespace gfx {
class ImageSkia;
}

namespace base {
class OneShotTimer;
}

class Profile;

class DownloadFeedbackView : public views::BubbleDelegateView,
                                                          public views::LinkListener {
 public:
  enum CloseReason
  {
    CLOSE_REASON_NONE,
    CLOSE_REASON_OPEN,
    CLOSE_REASON_OPEN_FOLDERS,
    CLOSE_REASON_RETRY,
    CLOSE_REASON_DELETE,
    CLOSE_REASON_OTHER
  };
  #if defined(OS_WIN)
    typedef base::Callback<void(CloseReason, bool, uint32_t)> CloseCb_CALLBACK;
  #else
    typedef base::Callback<void(CloseReason, bool, uint32_t)> CALLBACK;
  #endif

  enum DownloadState
  {
    DOWNLOAD_STATE_NONE,
    DOWNLOAD_SUCCESS,
    DOWNLOAD_FAIL
  };

  struct DownloadInfo
  {
    Browser* browser;
    gfx::ImageSkia image;
    DownloadState state;
    base::string16 file_name;
    base::string16 file_size;
    uint32_t item_id;
    #if defined(OS_WIN)
      CloseCb_CALLBACK close_cb;
    #else
      CALLBACK close_cb;
    #endif
    DownloadInfo();
    DownloadInfo(const DownloadInfo& info);
    ~DownloadInfo();
  };

  ~DownloadFeedbackView() override;

  // If |anchor_view| is null, |anchor_rect| is used to anchor the bubble and
  // |parent_window| is used to ensure the bubble closes if the parent closes.
  static views::Widget* ShowBubble(views::View* anchor_view,
                         const gfx::Rect& anchor_rect,
                         gfx::NativeView parent_window,
                         const DownloadInfo& info);

  static void Hide();

  static DownloadFeedbackView* download_bubble() { return download_bubble_; }

  // views::WidgetDelegate:
  void WindowClosing() override;
  bool ShouldShowCloseButton() const override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool CanActivate() const override;

  //views::BubbleDelegateView
  bool WantsMouseEventsWhenInactive() const override;

  // views::View:
  gfx::Size GetPreferredSize() const override;
  //void GetAccessibleState(ui::AXViewState* state) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  protected:
  // views::BubbleDelegateView method.
  void Init() override;

 private:
   // views::BubbleDelegateView:
  const char* GetClassName() const override;
  views::View* GetInitiallyFocusedView() override;

  // Creates a DownloadFeedbackView.
  DownloadFeedbackView(views::View* anchor_view,
                                              const DownloadInfo& info);

  void Reset();
  void AutoCloseBubble();
  void StartAutoCloseTimer();

  // The download bubble, if we're showing one.
  static DownloadFeedbackView* download_bubble_;

  DownloadInfo downloadInfo_;

  views::ImageView* image_;
  views::Label* status_label_;
  views::Label* file_label_;
  views::Label* size_label_;
  views::Link* left_link_;
  views::Link* right_link_;
  views::Checkbox* checkbox_view_;

  CloseReason close_reason_;

  base::OneShotTimer auto_close_timer_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFeedbackView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_FEEDBACK_VIEW_H_
