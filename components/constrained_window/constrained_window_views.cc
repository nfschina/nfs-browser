// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/constrained_window/constrained_window_views.h"

#include <algorithm>

#include "base/macros.h"
#include "build/build_config.h"
#include "components/constrained_window/constrained_window_views_client.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "ui/views/border.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/dialog_delegate.h"

#if defined(OS_MACOSX)
#import "components/constrained_window/native_web_contents_modal_dialog_manager_views_mac.h"
#endif

#if defined(OS_WIN)
#include "ui/display/win/dpi.h"
#include "ui/display/win/screen_win.h"
#endif

using web_modal::ModalDialogHost;
using web_modal::ModalDialogHostObserver;

namespace constrained_window {
namespace {

ConstrainedWindowViewsClient* constrained_window_views_client = NULL;

const int kDialogMargin = 2;

// The name of a key to store on the window handle to associate
// WidgetModalDialogHostObserverViews with the Widget.
const char* const kWidgetModalDialogHostObserverViewsKey =
    "__WIDGET_MODAL_DIALOG_HOST_OBSERVER_VIEWS__";

// Applies positioning changes from the ModalDialogHost to the Widget.
class WidgetModalDialogHostObserverViews
    : public views::WidgetObserver,
      public ModalDialogHostObserver {
 public:
  WidgetModalDialogHostObserverViews(ModalDialogHost* host,
                                     views::Widget* target_widget,
                                     const char *const native_window_property,
                                     bool is_nfs,
                                     gfx::Point offset)
      : host_(host),
        target_widget_(target_widget),
        native_window_property_(native_window_property),
        is_nfs_(is_nfs),
        pos_offset_(offset) {
    DCHECK(host_);
    DCHECK(target_widget_);
    host_->AddObserver(this);
    target_widget_->AddObserver(this);
  }

  ~WidgetModalDialogHostObserverViews() override {
    if (host_)
      host_->RemoveObserver(this);
    target_widget_->RemoveObserver(this);
    target_widget_->SetNativeWindowProperty(native_window_property_, NULL);
  }

  // WidgetObserver overrides
  void OnWidgetClosing(views::Widget* widget) override { delete this; }

  // WebContentsModalDialogHostObserver overrides
  void OnPositionRequiresUpdate() override {
    if (!is_nfs_) {
      UpdateWidgetModalDialogPosition(target_widget_, host_);
    } else {
      UpdateNfsWidgetModalDialogPosition(target_widget_, host_, pos_offset_);
    }
  }

  void OnHostDestroying() override {
    host_->RemoveObserver(this);
    host_ = NULL;
  }

  void OnHostMove() override {
    if (is_nfs_) {
      UpdateNfsWidgetModalDialogPosition(target_widget_, host_, pos_offset_, true);
    }
  }

 private:
  ModalDialogHost* host_;
  views::Widget* target_widget_;
  const char* const native_window_property_;
  bool is_nfs_;
  gfx::Point pos_offset_;

  DISALLOW_COPY_AND_ASSIGN(WidgetModalDialogHostObserverViews);
};

void UpdateModalDialogPosition(views::Widget* widget,
                               web_modal::ModalDialogHost* dialog_host,
                               const gfx::Size& size) {
  // Do not forcibly update the dialog widget position if it is being dragged.
  if (widget->HasCapture())
    return;

  views::Widget* host_widget =
      views::Widget::GetWidgetForNativeView(dialog_host->GetHostView());

  // If the host view is not backed by a Views::Widget, just update the widget
  // size. This can happen on MacViews under the Cocoa browser where the window
  // modal dialogs are displayed as sheets, and their position is managed by a
  // ConstrainedWindowSheetController instance.
  if (!host_widget) {
    widget->SetSize(size);
    return;
  }

  gfx::Point position = dialog_host->GetDialogPosition(size);
  views::Border* border = widget->non_client_view()->frame_view()->border();
  // Border may be null during widget initialization.
  if (border) {
    // Align the first row of pixels inside the border. This is the apparent
    // top of the dialog.
    position.set_y(position.y() - border->GetInsets().top());
  }

  if (widget->is_top_level())
    position += host_widget->GetClientAreaBoundsInScreen().OffsetFromOrigin();

  widget->SetBounds(gfx::Rect(position, size));
}

}  // namespace

// static
void SetConstrainedWindowViewsClient(
    std::unique_ptr<ConstrainedWindowViewsClient> new_client) {
  delete constrained_window_views_client;
  constrained_window_views_client = new_client.release();
}

void UpdateWebContentsModalDialogPosition(
    views::Widget* widget,
    web_modal::WebContentsModalDialogHost* dialog_host) {
  gfx::Size size = widget->GetRootView()->GetPreferredSize();
  gfx::Size max_size = dialog_host->GetMaximumDialogSize();
  // Enlarge the max size by the top border, as the dialog will be shifted
  // outside the area specified by the dialog host by this amount later.
  views::Border* border =
      widget->non_client_view()->frame_view()->border();
  // Border may be null during widget initialization.
  if (border)
    max_size.Enlarge(0, border->GetInsets().top());
  size.SetToMin(max_size);
  UpdateModalDialogPosition(widget, dialog_host, size);
}

void UpdateWidgetModalDialogPosition(views::Widget* widget,
                                     web_modal::ModalDialogHost* dialog_host) {
  UpdateModalDialogPosition(widget, dialog_host,
                            widget->GetRootView()->GetPreferredSize());
}

void UpdateNfsWidgetModalDialogPosition(views::Widget* widget,
                               web_modal::ModalDialogHost* dialog_host, gfx::Point offset,
                               bool is_move) {
  // Do not forcibly update the dialog widget position if it is being dragged.
  if (widget->HasCapture())
    return;

  gfx::Point position;
  bool out_of_screen = false;
  bool out_of_top = false;
  views::Widget* host_widget = views::Widget::GetWidgetForNativeView(
        dialog_host->GetHostView());
  gfx::Rect host_bounds = host_widget->GetClientAreaBoundsInScreen();
  gfx::Size size = widget->GetRootView()->GetPreferredSize();
  gfx::Rect workArea = views::Widget::GetWidgetForNativeView(
        dialog_host->GetHostView())->GetWorkAreaBoundsInScreen();
  int host_x = host_bounds.x();
  int valid_x = host_x;
  int host_y = host_bounds.y();
  int valid_y = host_y;
  int valid_height = host_bounds.height();
  int valid_width = host_bounds.width();

  #if defined(OS_WIN)
  int left_bound = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int right_bound = GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1;

  MONITORINFO monitor_info_left;
  MONITORINFO monitor_info_right;
  monitor_info_left.cbSize = sizeof(monitor_info_left);
  monitor_info_right.cbSize = sizeof(monitor_info_right);
  DWORD flags = 0;
  HMONITOR hm_left = MonitorFromPoint(gfx::Point(left_bound, 0).ToPOINT(), flags);
  HMONITOR hm_right = MonitorFromPoint(gfx::Point(right_bound, 0).ToPOINT(), flags);
  GetMonitorInfo(hm_left, &monitor_info_left);
  GetMonitorInfo(hm_right, &monitor_info_right);
  gfx::Rect pixel_bounds = gfx::Rect(monitor_info_left.rcWork);
  gfx::Rect DipRc_left = display::win::ScreenWin::ScreenToDIPRect(0, pixel_bounds);
  pixel_bounds = gfx::Rect(monitor_info_right.rcWork);
  gfx::Rect DipRc_right = display::win::ScreenWin::ScreenToDIPRect(0, pixel_bounds);
  int workAreas_left = DipRc_left.x();
  int workAreas_right = DipRc_left.x() + DipRc_left.width();
  if (hm_left != hm_right) {
    workAreas_right = DipRc_right.x() + DipRc_right.width();
  }
  #endif

  if (offset.x() >= 0 && offset.y() >= 0) {
    position = gfx::Point(host_bounds.x() + offset.x(), host_bounds.y() + offset.y());
  } else {
    //Center the widget.
    #if defined(OS_WIN)
    if (host_bounds.x() < workAreas_left) {
    #else
    if (host_bounds.x() < workArea.x()) {
    #endif
      valid_x =  workArea.x() + kDialogMargin;
      valid_width = host_bounds.width() + host_bounds.x() - workArea.x();
      if (valid_width < size.width()) {
        out_of_screen = true;
        #if defined(OS_LINUX)
        valid_x -= size.width() - valid_width;
        #endif
        valid_width = size.width();
        host_x = workArea.x() + size.width() - host_bounds.width();
      }
    }
    #if defined(OS_WIN)
    else if (host_bounds.x() + host_bounds.width() > workAreas_right) {
    #else
    else if (host_bounds.x() + host_bounds.width() > workArea.width()) {
    #endif
      valid_x += kDialogMargin;
      valid_width = workArea.x() + workArea.width() - host_bounds.x();
      if (valid_width < size.width()) {
        out_of_screen = true;
        valid_width = size.width();
        host_x = workArea.x() + workArea.width() - size.width();
      }
    }
    if (host_bounds.y() < workArea.y()) {
      out_of_top = true;
      host_y = 0;
    } else if (host_bounds.y() + host_bounds.height() > workArea.height()) {
      valid_y += 15;
      valid_height = workArea.height() - host_bounds.y();
      if (valid_height < size.height()) {
        out_of_screen = true;
        valid_height = size.height();
        host_y = workArea.height() - size.height();
      }
    }

    position = gfx::Point(valid_x + valid_width / 2 - size.width() / 2,
            valid_y + valid_height / 2 - size.height() /2);
  }

    widget->SetBounds(gfx::Rect(position, size));

    if ((!is_move && out_of_screen) || out_of_top) {
      host_widget->SetBounds(gfx::Rect(
              host_x, host_y, host_bounds.width(), host_bounds.height()));
    }
}

content::WebContents* GetTopLevelWebContents(
    content::WebContents* initiator_web_contents) {
  return guest_view::GuestViewBase::GetTopLevelWebContents(
      initiator_web_contents);
}

views::Widget* ShowWebModalDialogViews(
    views::WidgetDelegate* dialog,
    content::WebContents* initiator_web_contents) {
  DCHECK(constrained_window_views_client);
  // For embedded WebContents, use the embedder's WebContents for constrained
  // window.
  content::WebContents* web_contents =
      GetTopLevelWebContents(initiator_web_contents);
  views::Widget* widget = CreateWebModalDialogViews(dialog, web_contents);
  ShowModalDialog(widget->GetNativeWindow(), web_contents);
  return widget;
}


#if defined(OS_MACOSX)
views::Widget* ShowWebModalDialogWithOverlayViews(
    views::WidgetDelegate* dialog,
    content::WebContents* initiator_web_contents) {
  DCHECK(constrained_window_views_client);
  // For embedded WebContents, use the embedder's WebContents for constrained
  // window.
  content::WebContents* web_contents =
      GetTopLevelWebContents(initiator_web_contents);
  views::Widget* widget = CreateWebModalDialogViews(dialog, web_contents);
  web_modal::WebContentsModalDialogManager* manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents);
  std::unique_ptr<web_modal::SingleWebContentsDialogManager> dialog_manager(
      new NativeWebContentsModalDialogManagerViewsMac(widget->GetNativeWindow(),
                                                      manager));
  manager->ShowDialogWithManager(widget->GetNativeWindow(),
                                 std::move(dialog_manager));
  return widget;
}
#endif

views::Widget* CreateWebModalDialogViews(views::WidgetDelegate* dialog,
                                         content::WebContents* web_contents) {
  DCHECK_EQ(ui::MODAL_TYPE_CHILD, dialog->GetModalType());
  return views::DialogDelegate::CreateDialogWidget(
      dialog, nullptr,
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents)
          ->delegate()
          ->GetWebContentsModalDialogHost()
          ->GetHostView());
}

static views::Widget* CreateBrowserModalDialogViewsImp(views::DialogDelegate* dialog,
                                             gfx::NativeWindow parent, bool is_nfs = false,
                                             gfx::Point offset = gfx::Point(-1, -1)) {
  DCHECK_NE(ui::MODAL_TYPE_CHILD, dialog->GetModalType());
  DCHECK_NE(ui::MODAL_TYPE_NONE, dialog->GetModalType());

  DCHECK(constrained_window_views_client);
  gfx::NativeView parent_view =
      parent ? constrained_window_views_client->GetDialogHostView(parent)
             : nullptr;
  views::Widget* widget;
  if (!is_nfs) {
    widget = views::DialogDelegate::CreateDialogWidget(dialog, NULL, parent_view);
  } else {
    widget = views::NfsDialogDelegate::CreateNfsDialogWidget(dialog, NULL, parent_view);
  }

  bool requires_positioning = dialog->ShouldUseCustomFrame();

#if defined(OS_MACOSX)
  // On Mac, window modal dialogs are displayed as sheets, so their position is
  // managed by the parent window.
  requires_positioning = false;
#endif

  if (!requires_positioning)
    return widget;

  ModalDialogHost* host = constrained_window_views_client->
      GetModalDialogHost(parent);
  if (host) {
    DCHECK_EQ(parent_view, host->GetHostView());
    ModalDialogHostObserver* dialog_host_observer =
        new WidgetModalDialogHostObserverViews(
            host, widget, kWidgetModalDialogHostObserverViewsKey, is_nfs, offset);
    dialog_host_observer->OnPositionRequiresUpdate();
  }
  return widget;
}

views::Widget* CreateBrowserModalDialogViews(views::DialogDelegate* dialog,
                                             gfx::NativeWindow parent) {
  return CreateBrowserModalDialogViewsImp(dialog, parent);
}

views::Widget* CreateNfsModalDialogViews(views::DialogDelegate* dialog,
                                             gfx::NativeWindow parent, gfx::Point offset) {
  return CreateBrowserModalDialogViewsImp(dialog, parent, true, offset);
}

}  // namespace constrained window
