// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"

#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/nfs_sync/nfs_sync_service.h"
#include "chrome/browser/nfs_sync/nfs_sync_service_factory.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_platform_specific.h"
#include "chrome/browser/ui/views/profiles/profile_indicator_icon.h"
#include "chrome/browser/ui/views/tab_icon_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/hit_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/path.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/views_delegate.h"
#include "ui/views/window/frame_background.h"
#include "ui/views/window/window_shape.h"

#if defined(OS_LINUX)
#include "ui/views/controls/menu/menu_runner.h"
#endif

#if defined(OS_WIN)
#include "ui/display/win/screen_win.h"
#endif

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/command_updater.h"
#include "grit/ui_resources_nfs.h"
#include "chrome/browser/profiles/profile_manager.h"

using content::WebContents;

namespace {

// In the window corners, the resize areas don't actually expand bigger, but the
// 16 px at the end of each edge triggers diagonal resizing.
const int kResizeAreaCornerSize = 16;

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, public:

OpaqueBrowserFrameView::OpaqueBrowserFrameView(BrowserFrame* frame,
                                               BrowserView* browser_view)
    : BrowserNonClientFrameView(frame, browser_view),
      layout_(new OpaqueBrowserFrameViewLayout(this)),
      minimize_button_(nullptr),
      maximize_button_(nullptr),
      restore_button_(nullptr),
      close_button_(nullptr),
      pin_button_(nullptr),
      unpin_button_(nullptr),
      return_button_(nullptr),
      theme_button_(nullptr),
      account_button_(nullptr),
      window_icon_(nullptr),
      window_title_(nullptr),
      profile_switcher_(this),
      frame_background_(new views::FrameBackground()),
      is_from_simple_web_(false),
      is_simple_web_vedio_(browser_view->browser()->is_simple_web_vedio()),
      browser_view_(browser_view) {
    SetLayoutManager(layout_);
    NfsSyncService* sync_service =
        NfsSyncServiceFactory::GetForBrowserContext(
            ProfileManager::GetPrimaryUserProfile());
    sync_service->AddObserver(this);

    if(browser_view->browser()->is_simple_web_vedio()) {
    minimize_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_FRAME_MIN,
                                             IDD_WEB_VIDEO_FRAME_MIN_H,
                                             IDD_WEB_VIDEO_FRAME_MIN_P,
                                             IDR_MINIMIZE_BUTTON_MASK,
                                             IDS_ACCNAME_MINIMIZE,
                                             VIEW_ID_MINIMIZE_BUTTON);
    maximize_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_FRAME_MAX,
                                             IDD_WEB_VIDEO_FRAME_MAX_H,
                                             IDD_WEB_VIDEO_FRAME_MAX_P,
                                             IDR_MAXIMIZE_BUTTON_MASK,
                                             IDS_ACCNAME_MAXIMIZE,
                                             VIEW_ID_MAXIMIZE_BUTTON);
    restore_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_FRAME_RESTORE,
                                            IDD_WEB_VIDEO_FRAME_RESTORE_H,
                                            IDD_WEB_VIDEO_FRAME_RESTORE_P,
                                            IDR_RESTORE_BUTTON_MASK,
                                            IDS_ACCNAME_RESTORE,
                                            VIEW_ID_RESTORE_BUTTON);
    close_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_FRAME_CLOSE,
                                          IDD_WEB_VIDEO_FRAME_CLOSE_H,
                                          IDD_WEB_VIDEO_FRAME_CLOSE_P,
                                          IDR_CLOSE_BUTTON_MASK,
                                          IDS_ACCNAME_CLOSE,
                                          VIEW_ID_CLOSE_BUTTON);
    pin_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_PIN,
                                          IDD_WEB_VIDEO_PIN_H,
                                          IDD_WEB_VIDEO_PIN_P,
                                          IDR_CLOSE_BUTTON_MASK,
                                          IDS_ACCNAME_CLOSE,
                                          VIEW_ID_WEB_VIDEO_PIN_BUTTON);
    unpin_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_UNPIN,
                                          IDD_WEB_VIDEO_UNPIN_H,
                                          IDD_WEB_VIDEO_UNPIN_P,
                                          IDR_CLOSE_BUTTON_MASK,
                                          IDS_ACCNAME_CLOSE,
                                          VIEW_ID_WEB_VIDEO_UNPIN_BUTTON);
    return_button_ = InitWindowCaptionButton(IDD_WEB_VIDEO_RETURN,
                                          IDD_WEB_VIDEO_RETURN_H,
                                          IDD_WEB_VIDEO_RETURN_P,
                                          IDR_CLOSE_BUTTON_MASK,
                                          IDS_ACCNAME_CLOSE,
                                          VIEW_ID_WEB_VIDEO_RETURN_BUTTON);
    } else {
      if (!frame->GetThemeProvider()->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK)) {
        if(browser_view_->browser()->is_type_tabbed()) {
          theme_button_ = InitWindowCaptionButton(IDD_THEME_GALLERY_N,
                                                 IDD_THEME_GALLERY_H,
                                                 IDD_THEME_GALLERY_H,
                                                 0, //this id is not important,see implemention of InitWindowCaptionButton.
                                                 IDS_ACCNAME_THEME,
                                                 VIEW_ID_THEME_BUTTON);

          account_button_ = InitWindowCaptionButton(IDD_ACCOUNT_UNLOGIN,
                                          IDD_ACCOUNT_UNLOGIN_H,
                                          IDD_ACCOUNT_UNLOGIN_P,
                                          0, //this id is not important,see implemention of InitWindowCaptionButton.
                                          IDS_ACCNAME_CLOSE,
                                          VIEW_ID_ACCOUNT_BUTTON);
        }

        minimize_button_ = InitWindowCaptionButton(IDD_FRAME_MIN,
                                                 IDD_FRAME_MIN_H,
                                                 IDD_FRAME_MIN_P,
                                                 IDR_MINIMIZE_BUTTON_MASK,
                                                 IDS_ACCNAME_MINIMIZE,
                                                 VIEW_ID_MINIMIZE_BUTTON);
        maximize_button_ = InitWindowCaptionButton(IDD_FRAME_MAX,
                                                 IDD_FRAME_MAX_H,
                                                 IDD_FRAME_MAX_P,
                                                 IDR_MAXIMIZE_BUTTON_MASK,
                                                 IDS_ACCNAME_MAXIMIZE,
                                                 VIEW_ID_MAXIMIZE_BUTTON);

        restore_button_ = InitWindowCaptionButton(IDD_FRAME_RESTORE,
                                                IDD_FRAME_RESTORE_H,
                                                IDD_FRAME_RESTORE_P,
                                                IDR_RESTORE_BUTTON_MASK,
                                                IDS_ACCNAME_RESTORE,
                                                VIEW_ID_RESTORE_BUTTON);
        close_button_ = InitWindowCaptionButton(IDD_FRAME_CLOSE,
                                              IDD_FRAME_CLOSE_H,
                                              IDD_FRAME_CLOSE_P,
                                              IDR_CLOSE_BUTTON_MASK,
                                              IDS_ACCNAME_CLOSE,
                                              VIEW_ID_CLOSE_BUTTON);
      } else {
        if(browser_view_->browser()->is_type_tabbed()) {
          theme_button_ = InitWindowCaptionButton(IDD_THEME_GALLERY_N_DARK,
                                                   IDD_THEME_GALLERY_H_DARK,
                                                   IDD_THEME_GALLERY_H_DARK,
                                                   0, //this id is not important,see implemention of InitWindowCaptionButton.
                                                   IDS_ACCNAME_THEME,
                                                   VIEW_ID_THEME_BUTTON);
          account_button_ = InitWindowCaptionButton(IDD_ACCOUNT_UNLOGIN,
                                          IDD_ACCOUNT_UNLOGIN_H,
                                          IDD_ACCOUNT_UNLOGIN_P,
                                          0, //this id is not important,see implemention of InitWindowCaptionButton.
                                          IDS_ACCNAME_CLOSE,
                                          VIEW_ID_ACCOUNT_BUTTON);
        }

        minimize_button_ = InitWindowCaptionButton(IDD_FRAME_MIN_DARK,
                                             IDD_FRAME_MIN_H_DARK,
                                             IDD_FRAME_MIN_P_DARK,
                                             IDR_MINIMIZE_BUTTON_MASK,
                                             IDS_ACCNAME_MINIMIZE,
                                             VIEW_ID_MINIMIZE_BUTTON);
        maximize_button_ = InitWindowCaptionButton(IDD_FRAME_MAX_DARK,
                                                   IDD_FRAME_MAX_H_DARK,
                                                   IDD_FRAME_MAX_P_DARK,
                                                   IDR_MAXIMIZE_BUTTON_MASK,
                                                   IDS_ACCNAME_MAXIMIZE,
                                                   VIEW_ID_MAXIMIZE_BUTTON);

        restore_button_ = InitWindowCaptionButton(IDD_FRAME_RESTORE_DARK,
                                                  IDD_FRAME_RESTORE_H_DARK,
                                                  IDD_FRAME_RESTORE_P_DARK,
                                                  IDR_RESTORE_BUTTON_MASK,
                                                  IDS_ACCNAME_RESTORE,
                                                  VIEW_ID_RESTORE_BUTTON);
        close_button_ = InitWindowCaptionButton(IDD_FRAME_CLOSE_DARK,
                                                IDD_FRAME_CLOSE_H_DARK,
                                                IDD_FRAME_CLOSE_P_DARK,
                                                IDR_CLOSE_BUTTON_MASK,
                                                IDS_ACCNAME_CLOSE,
                                                VIEW_ID_CLOSE_BUTTON);
      }
  }

  // Initializing the TabIconView is expensive, so only do it if we need to.
  if (browser_view->ShouldShowWindowIcon()) {
    window_icon_ = new TabIconView(this, this);
    window_icon_->set_is_light(true);
    window_icon_->set_id(VIEW_ID_WINDOW_ICON);
    AddChildView(window_icon_);
    window_icon_->Update();
  }

  window_title_ = new views::Label(
      browser_view->GetWindowTitle(),
      gfx::FontList(BrowserFrame::GetTitleFontList()));
  window_title_->SetVisible(browser_view->ShouldShowWindowTitle());

  if (browser_view->browser()->is_simple_web_download_manager()) {
    window_title_->SetAutoColorReadabilityEnabled(false);
    if (frame->GetThemeProvider()->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK)) {
      window_title_->SetEnabledColor(SK_ColorBLACK);
    } else {
      window_title_->SetEnabledColor(SK_ColorWHITE);
    }
  }

  if(browser_view->browser()->is_simple_web_vedio()) {
    window_title_->SetBackgroundColor(SK_ColorBLACK);
  }
  window_title_->SetSubpixelRenderingEnabled(false);
  window_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  window_title_->set_id(VIEW_ID_WINDOW_TITLE);
  AddChildView(window_title_);

  platform_observer_.reset(OpaqueBrowserFrameViewPlatformSpecific::Create(
      this, layout_,
      ThemeServiceFactory::GetForProfile(browser_view->browser()->profile())));

  // set the tooltips
  if (pin_button_)  {
    pin_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_SMALL_POPUP_VIDEO_CANCEL_TOPMOST_BTN));
   }

  if (unpin_button_)  {
    unpin_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_SMALL_POPUP_VIDEO_TOPMOST_BTN));
   }

  if (return_button_)  {
    return_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_SMALL_POPUP_VIDEO_RETURN_BTN));
   }

   if (theme_button_) {
      theme_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_THEME_BUTTON_TOOLTIP));
   }

  if (account_button_) {
      account_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_LOGIN_BUTTON_TOOLTIP));
   }

   if (minimize_button_) {
      minimize_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_TOOLTIP_MINIMIZE_BUTTON));
   }

   if (maximize_button_) {
      maximize_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_TOOLTIP_MAXIMIZE_BUTTON));
   }

   if (restore_button_) {
      restore_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_TOOLTIP_RESTORE_BUTTON));
   }

   if (close_button_) {
      close_button_->SetTooltipText(l10n_util::GetStringUTF16(
          IDS_TOOLTIP_CLOSE_BUTTON));
   }

  if (account_button_ && sync_service->IsLoggedIn()) {
    const ui::ThemeProvider* tp = frame->GetThemeProvider();
    account_button_->SetImage(views::CustomButton::STATE_NORMAL,
                       tp->GetImageSkiaNamed(IDD_ACCOUNT_SYNC));
    account_button_->SetImage(views::CustomButton::STATE_HOVERED,
                       tp->GetImageSkiaNamed(IDD_ACCOUNT_SYNC_H));
    account_button_->SetImage(views::CustomButton::STATE_PRESSED,
                       tp->GetImageSkiaNamed(IDD_ACCOUNT_SYNC_P));
    nfs_sync::AccountInfo account_info = sync_service->GetAccountInfo();
    account_button_->SetTooltipText(base::UTF8ToUTF16(account_info.name));
  }

  is_from_simple_web_ = browser_view->browser()->is_type_simple_web();
}

OpaqueBrowserFrameView::~OpaqueBrowserFrameView() {
    NfsSyncService* sync_service =
        NfsSyncServiceFactory::GetForBrowserContext(
            ProfileManager::GetPrimaryUserProfile());
    if (sync_service) {
      sync_service->RemoveObserver(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, BrowserNonClientFrameView implementation:

gfx::Rect OpaqueBrowserFrameView::GetBoundsForTabStrip(
    views::View* tabstrip) const {
  if (!tabstrip)
    return gfx::Rect();

  return layout_->GetBoundsForTabStrip(tabstrip->GetPreferredSize(), width());
}

int OpaqueBrowserFrameView::GetTopInset(bool restored) const {
  return browser_view()->IsTabStripVisible()
             ? layout_->GetTabStripInsetsTop(restored)
             : layout_->NonClientTopHeight(restored);
}

int OpaqueBrowserFrameView::GetThemeBackgroundXInset() const {
  return 0;
}

void OpaqueBrowserFrameView::UpdateThrobber(bool running) {
  if (window_icon_)
    window_icon_->Update();
}

gfx::Size OpaqueBrowserFrameView::GetMinimumSize() const {
  return layout_->GetMinimumSize(width());
}

views::View* OpaqueBrowserFrameView::GetProfileSwitcherView() const {
  return profile_switcher_.view();
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, views::NonClientFrameView implementation:

gfx::Rect OpaqueBrowserFrameView::GetBoundsForClientView() const {
  return layout_->client_view_bounds();
}

gfx::Rect OpaqueBrowserFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return layout_->GetWindowBoundsForClientBounds(client_bounds);
}

bool OpaqueBrowserFrameView::IsWithinAvatarMenuButtons(
    const gfx::Point& point) const {
  if (profile_indicator_icon() &&
      profile_indicator_icon()->GetMirroredBounds().Contains(point)) {
    return true;
  }
  if (profile_switcher_.view() &&
      profile_switcher_.view()->GetMirroredBounds().Contains(point)) {
    return true;
  }

  return false;
}

int OpaqueBrowserFrameView::NonClientHitTest(const gfx::Point& point) {
  if (!bounds().Contains(point))
    return HTNOWHERE;

  // See if the point is within the avatar menu button.
  if (IsWithinAvatarMenuButtons(point))
    return HTCLIENT;

  int frame_component = frame()->client_view()->NonClientHitTest(point);

  // See if we're in the sysmenu region.  We still have to check the tabstrip
  // first so that clicks in a tab don't get treated as sysmenu clicks.
  gfx::Rect sysmenu_rect(IconBounds());
  // In maximized mode we extend the rect to the screen corner to take advantage
  // of Fitts' Law.
  if (layout_->IsTitleBarCondensed())
    sysmenu_rect.SetRect(0, 0, sysmenu_rect.right(), sysmenu_rect.bottom());
  sysmenu_rect.set_x(GetMirroredXForRect(sysmenu_rect));
  if (sysmenu_rect.Contains(point))
    return (frame_component == HTCLIENT) ? HTCLIENT : HTSYSMENU;

  if (frame_component != HTNOWHERE)
    return frame_component;

  // Then see if the point is within any of the window controls.
  if (close_button_ && close_button_->visible() &&
      close_button_->GetMirroredBounds().Contains(point))
    return HTCLOSE;
  if (restore_button_ && restore_button_->visible() &&
      restore_button_->GetMirroredBounds().Contains(point))
    return HTMAXBUTTON;
  if (maximize_button_ && maximize_button_->visible() &&
      maximize_button_->GetMirroredBounds().Contains(point))
    return HTMAXBUTTON;
  if (minimize_button_ && minimize_button_->visible() &&
      minimize_button_->GetMirroredBounds().Contains(point))
    return HTMINBUTTON;
  if (theme_button_ && theme_button_->visible() &&
      theme_button_->GetMirroredBounds().Contains(point))
    return HTTHEMEBUTTON;
  if (account_button_ && account_button_->visible() &&
      account_button_->GetMirroredBounds().Contains(point))
    return HTACCOUNTBUTTON;
  if (pin_button_ && pin_button_->visible() &&
      pin_button_->GetMirroredBounds().Contains(point))
    return HTPIN;
  if (unpin_button_ && unpin_button_->visible() &&
      unpin_button_->GetMirroredBounds().Contains(point))
    return HTUNPIN;
  if (return_button_ && return_button_->visible() &&
      return_button_->GetMirroredBounds().Contains(point))
    return HTRETURN;

  views::WidgetDelegate* delegate = frame()->widget_delegate();
  if (!delegate) {
    LOG(WARNING) << "delegate is null, returning safe default.";
    return HTCAPTION;
  }
  int window_component = GetHTComponentForFrame(
      point, FrameBorderThickness(false), NonClientBorderThickness(),
      kResizeAreaCornerSize, kResizeAreaCornerSize, delegate->CanResize());
  // Fall back to the caption if no other component matches.
  return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

void OpaqueBrowserFrameView::GetWindowMask(const gfx::Size& size,
                                           gfx::Path* window_mask) {
  DCHECK(window_mask);

  if (layout_->IsTitleBarCondensed() || frame()->IsFullscreen())
    return;

#if defined(OS_WIN)
  // we don't want the round corner
  views::GetWindowMaskWithoutRoundCorner(
      size, frame()->GetCompositor()->device_scale_factor(), window_mask);
#endif
}

void OpaqueBrowserFrameView::ResetWindowControls() {
  if(restore_button_) {
    restore_button_->SetState(views::CustomButton::STATE_NORMAL);
  }
  if(minimize_button_) {
    minimize_button_->SetState(views::CustomButton::STATE_NORMAL);
  }
  if(maximize_button_) {
    maximize_button_->SetState(views::CustomButton::STATE_NORMAL);
  }
  if(close_button_) {
    close_button_->SetState(views::CustomButton::STATE_NORMAL);
  }
  if (theme_button_) {
    theme_button_->SetState(views::CustomButton::STATE_NORMAL);
  }
  // The close button isn't affected by this constraint.
}

void OpaqueBrowserFrameView::UpdateWindowIcon() {
  if (window_icon_)
    window_icon_->SchedulePaint();
}

void OpaqueBrowserFrameView::UpdateWindowTitle() {
  if (!frame()->IsFullscreen())
    window_title_->SchedulePaint();
}

void OpaqueBrowserFrameView::SizeConstraintsChanged() {
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, views::View overrides:

void OpaqueBrowserFrameView::GetAccessibleState(
    ui::AXViewState* state) {
  state->role = ui::AX_ROLE_TITLE_BAR;
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, views::ButtonListener implementation:

void OpaqueBrowserFrameView::ButtonPressed(views::Button* sender,
                                           const ui::Event& event) {
  if (sender == minimize_button_) {
    frame()->Minimize();
  } else if (sender == maximize_button_) {
    frame()->Maximize();
  } else if (sender == restore_button_) {
    frame()->Restore();
  } else if (sender == close_button_) {
    frame()->Close();
  } else if (sender == unpin_button_) {
    frame()->SetAlwaysOnTop(true);
    Layout();
  }else if (sender == pin_button_) {
    frame()->SetAlwaysOnTop(false);
    Layout();
  } else if (sender == return_button_) {
    browser_view()->browser()->command_controller()->command_updater()->ExecuteCommand(IDC_ATTACH_WEB_VEDIO);
  } else if (sender == theme_button_) {
    browser_view()->browser()->command_controller()->command_updater()->ExecuteCommand(IDC_THEME_GALLERY);
  } else if (sender == account_button_) {
    browser_view()->browser()->command_controller()->command_updater()->ExecuteCommand(IDC_ACCOUNT_DIALOG);
  }
}

void OpaqueBrowserFrameView::OnMenuButtonClicked(views::MenuButton* source,
                                                 const gfx::Point& point,
                                                 const ui::Event* event) {
#if defined(OS_LINUX)
  views::MenuRunner menu_runner(frame()->GetSystemMenuModel(),
                                views::MenuRunner::HAS_MNEMONICS);
  ignore_result(menu_runner.RunMenuAt(browser_view()->GetWidget(),
                                      window_icon_,
                                      window_icon_->GetBoundsInScreen(),
                                      views::MENU_ANCHOR_TOPLEFT,
                                      ui::MENU_SOURCE_MOUSE));
#endif
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, TabIconView::TabContentsProvider implementation:

bool OpaqueBrowserFrameView::ShouldTabIconViewAnimate() const {
  // This function is queried during the creation of the window as the
  // TabIconView we host is initialized, so we need to null check the selected
  // WebContents because in this condition there is not yet a selected tab.
  WebContents* current_tab = browser_view()->GetActiveWebContents();
  return current_tab ? current_tab->IsLoading() : false;
}

gfx::ImageSkia OpaqueBrowserFrameView::GetFaviconForTabIconView() {
  views::WidgetDelegate* delegate = frame()->widget_delegate();
  if (!delegate) {
    LOG(WARNING) << "delegate is null, returning safe default.";
    return gfx::ImageSkia();
  }
  return delegate->GetWindowIcon();
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, OpaqueBrowserFrameViewLayoutDelegate implementation:

bool OpaqueBrowserFrameView::ShouldShowWindowIcon() const {
  views::WidgetDelegate* delegate = frame()->widget_delegate();
  return ShouldShowWindowTitleBar() && delegate &&
         delegate->ShouldShowWindowIcon();
}

bool OpaqueBrowserFrameView::ShouldShowWindowTitle() const {
  // |delegate| may be null if called from callback of InputMethodChanged while
  // a window is being destroyed.
  // See more discussion at http://crosbug.com/8958
  views::WidgetDelegate* delegate = frame()->widget_delegate();
  return ShouldShowWindowTitleBar() && delegate &&
         delegate->ShouldShowWindowTitle();
}

bool OpaqueBrowserFrameView::ShouldShowTabStripTnsertsTop() const {
  views::WidgetDelegate* delegate = frame()->widget_delegate();
  return delegate && delegate->ShouldShowTabStripTnsertsTop();
}

base::string16 OpaqueBrowserFrameView::GetWindowTitle() const {
  return frame()->widget_delegate()->GetWindowTitle();
}

int OpaqueBrowserFrameView::GetIconSize() const {
#if defined(OS_WIN)
  // This metric scales up if either the titlebar height or the titlebar font
  // size are increased.
  return display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSMICON);
#else
  // The icon never shrinks below 16 px on a side.
  const int kIconMinimumSize = 16;
  return std::max(BrowserFrame::GetTitleFontList().GetHeight(),
                  kIconMinimumSize);
#endif
}

gfx::Size OpaqueBrowserFrameView::GetBrowserViewMinimumSize() const {
  return browser_view()->GetMinimumSize();
}

bool OpaqueBrowserFrameView::ShouldShowCaptionButtons() const {
  return ShouldShowWindowTitleBar();
}

bool OpaqueBrowserFrameView::IsRegularOrGuestSession() const {
  return browser_view()->IsRegularOrGuestSession();
}

gfx::ImageSkia OpaqueBrowserFrameView::GetIncognitoAvatarIcon() const {
  return BrowserNonClientFrameView::GetIncognitoAvatarIcon();
}

bool OpaqueBrowserFrameView::IsMaximized() const {
  return frame()->IsMaximized();
}

bool OpaqueBrowserFrameView::IsMinimized() const {
  return frame()->IsMinimized();
}

bool OpaqueBrowserFrameView::IsAlwaysOnTop() const {
  return frame()->IsAlwaysOnTop();
}

bool OpaqueBrowserFrameView::IsFullscreen() const {
  return frame()->IsFullscreen();
}

bool OpaqueBrowserFrameView::IsTabStripVisible() const {
  return browser_view()->IsTabStripVisible();
}

bool OpaqueBrowserFrameView::IsToolbarVisible() const {
  return browser_view()->IsToolbarVisible() &&
      !browser_view()->toolbar()->GetPreferredSize().IsEmpty();
}

int OpaqueBrowserFrameView::GetTabStripHeight() const {
  return browser_view()->GetTabStripHeight();
}

gfx::Size OpaqueBrowserFrameView::GetTabstripPreferredSize() const {
  gfx::Size s = browser_view()->tabstrip()->GetPreferredSize();
  return s;
}

bool OpaqueBrowserFrameView::IsSimpleWebVedio() const {
  return is_simple_web_vedio_;
}

bool OpaqueBrowserFrameView::hasCustomImage() const {
  const ui::ThemeProvider* tp = frame()->GetThemeProvider();
  return tp && tp->HasCustomImage(IDR_THEME_TOOLBAR);
}

void OpaqueBrowserFrameView::OnLoginSuccess(const std::string& name,
                                            const std::string& email,
                                            const std::string& id) {
  if(!account_button_)
    return;

  const ui::ThemeProvider* tp = frame()->GetThemeProvider();
  account_button_->SetImage(views::CustomButton::STATE_NORMAL,
                      tp->GetImageSkiaNamed(IDD_ACCOUNT_SYNC));
  account_button_->SetImage(views::CustomButton::STATE_HOVERED,
                      tp->GetImageSkiaNamed(IDD_ACCOUNT_SYNC_H));
  account_button_->SetImage(views::CustomButton::STATE_PRESSED,
                      tp->GetImageSkiaNamed(IDD_ACCOUNT_SYNC_P));

  if (!name.empty()) {
    account_button_->SetTooltipText(base::UTF8ToUTF16(name));
  } else {
    account_button_->SetTooltipText(base::UTF8ToUTF16(id));
  }

  Layout();
}

void OpaqueBrowserFrameView::OnLogoutSuccess() {
  if(!account_button_)
    return;

  const ui::ThemeProvider* tp = frame()->GetThemeProvider();
  account_button_->SetImage(views::CustomButton::STATE_NORMAL,
                      tp->GetImageSkiaNamed(IDD_ACCOUNT_UNLOGIN));
  account_button_->SetImage(views::CustomButton::STATE_HOVERED,
                      tp->GetImageSkiaNamed(IDD_ACCOUNT_UNLOGIN_H));
  account_button_->SetImage(views::CustomButton::STATE_PRESSED,
                      tp->GetImageSkiaNamed(IDD_ACCOUNT_UNLOGIN_P));
  account_button_->SetTooltipText(l10n_util::GetStringUTF16(
            IDS_LOGIN_BUTTON_TOOLTIP));

  Layout();
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, protected:

// views::View:
void OpaqueBrowserFrameView::OnPaint(gfx::Canvas* canvas) {
  if (frame()->IsFullscreen())
    return;  // Nothing is visible, so don't bother to paint.

  if (layout_->IsTitleBarCondensed())
    PaintMaximizedFrameBorder(canvas);
  else
    PaintRestoredFrameBorder(canvas);

  // The window icon and title are painted by their respective views.
  /* TODO(pkasting):  If this window is active, we should also draw a drop
   * shadow on the title.  This is tricky, because we don't want to hardcode a
   * shadow color (since we want to work with various themes), but we can't
   * alpha-blend either (since the Windows text APIs don't really do this).
   * So we'd need to sample the background color at the right location and
   * synthesize a good shadow color. */

  if (IsToolbarVisible() && IsTabStripVisible())
    PaintToolbarBackground(canvas);

  const ui::ThemeProvider* tp = GetThemeProvider();
  bool has_custom_image = tp && tp->HasCustomImage(IDR_THEME_TOOLBAR);
  bool should_paint_client_edge =  !has_custom_image ||
                  browser_view_->browser()->is_simple_web_download_manager() ||
                  browser_view_->browser()->is_simple_web_new_download_task();
  if (should_paint_client_edge)
    PaintClientEdge(canvas);

  // normal state
 if (!has_custom_image && !layout_->IsTitleBarCondensed()) {
    gfx::Rect frame_rect = GetLocalBounds();
    const gfx::Point p1 = frame_rect.origin();
    const gfx::Point p2 = frame_rect.top_right();
    const gfx::Point p3 = frame_rect.bottom_left();
    const gfx::Point p4 = frame_rect.bottom_right();

    int height_tabstrip = 0;
    int height_toolbar = 0;
    int height_bookmark = 0;
    int height_client = 0;
    if (is_from_simple_web_) {
      height_client = GetTopAreaHeight();
    } else {
      gfx::Rect toolbar_bounds(browser_view()->GetToolbarBounds());
      gfx::Rect bookmark_bounds(browser_view()->GetBookmarkBounds());
      height_tabstrip = GetTopAreaHeight();
      height_toolbar = toolbar_bounds.height();
      height_bookmark = bookmark_bounds.height();
      height_client = height_tabstrip + height_toolbar + height_bookmark - 7;
    }


    SkColor line_color = SkColorSetRGB(180, 180, 180);
    SkColor background_color = SK_ColorWHITE;
    if (IsSimpleWebVedio()) {
      line_color = GetFrameColor();
      background_color = SK_ColorBLACK;
    }

    const float scale = GetWidget()->GetCompositor()->device_scale_factor();
    if(scale == 1) {
     canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p2, line_color);
     canvas->DrawLine(p1, p2 + gfx::Vector2d(-1 , 1), line_color);
     canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p2 + gfx::Vector2d(-1 , 1), GetFrameColor());

     canvas->DrawLine(p1 + gfx::Vector2d(1 , 30), p3 + gfx::Vector2d(1 , -1) , background_color);
     canvas->DrawLine(p1 + gfx::Vector2d(1 , 30), p1 + gfx::Vector2d(1 , 41) , GetFrameColor());
     canvas->DrawLine(p1, p3 + gfx::Vector2d(1 , -1), line_color);
     canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p3 , line_color);

     canvas->DrawLine(p2 + gfx::Vector2d(-1 , 1), p4 , line_color);
     canvas->DrawLine(p2 + gfx::Vector2d(0 , 1), p4 + gfx::Vector2d(-1 , -1), line_color);
     // huk 这个100是为了让线画出框外面，否则有点问题（非正统解法）
     canvas->DrawLine(p2 + gfx::Vector2d(-1 , is_from_simple_web_ ? 30 : 40), p4  + gfx::Vector2d(-2, 100), background_color);
     // canvas->DrawLine(p2 + gfx::Vector2d(0 , 30), p2 + gfx::Vector2d(-1 , 41) , GetFrameColor());

     canvas->DrawLine(p3 + gfx::Vector2d(0 , -1) , p4 + gfx::Vector2d(0, -1), line_color);
     canvas->DrawLine(p3 + gfx::Vector2d(1 , -2) , p4 + gfx::Vector2d(-1, -2), background_color);
     canvas->DrawLine(p3 + gfx::Vector2d(1 , -3) , p4 + gfx::Vector2d(-1, -3), background_color);
    }else {
    // 上
    // canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p2, line_color);
    // canvas->DrawLine(p1, p2 + gfx::Vector2d(-1 , 1), line_color);
    // canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p2 + gfx::Vector2d(-1 , 1), GetFrameColor());


    // 左
    //canvas->DrawLine(p1 + gfx::Vector2d(1 , 30), p3 + gfx::Vector2d(1 , -1) , background_color);
    //canvas->DrawLine(p1 + gfx::Vector2d(1 , 30), p1 + gfx::Vector2d(1 , 41) , GetFrameColor());
    // canvas->DrawLine(p1, p3 + gfx::Vector2d(1 , -1), line_color);
    // canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p3 , line_color);
    canvas->DrawLine(p1 + gfx::Vector2d(2 , height_tabstrip), p1 + gfx::Vector2d(2 , height_client),
        tp->GetColor(ThemeProperties::COLOR_TOOLBAR));
    canvas->DrawLine(p1 + gfx::Vector2d(1 , height_tabstrip), p1 + gfx::Vector2d(1 , height_client),
        tp->GetColor(ThemeProperties::COLOR_TOOLBAR));
    canvas->DrawLine(p1 + gfx::Vector2d(0 , height_tabstrip), p1 + gfx::Vector2d(0 , height_client),
        tp->GetColor(ThemeProperties::COLOR_TOOLBAR));

    canvas->DrawLine(p1 + gfx::Vector2d(2 , height_client), p3 + gfx::Vector2d(2 , 0), background_color);
    canvas->DrawLine(p1 + gfx::Vector2d(1 , height_client), p3 + gfx::Vector2d(1 , 0), background_color);
    canvas->DrawLine(p1 + gfx::Vector2d(0 , height_client), p3 + gfx::Vector2d(0 , 0), background_color);

    //canvas->DrawLine(p1 + gfx::Vector2d(1 , 1), p3 , line_color);

    // 右
    canvas->DrawLine(p2 + gfx::Vector2d(-2 , height_tabstrip), p4 + gfx::Vector2d(-2 , height_client),
        tp->GetColor(ThemeProperties::COLOR_TOOLBAR));
    canvas->DrawLine(p2 + gfx::Vector2d(-1 , height_tabstrip), p4 + gfx::Vector2d(-1 , height_client),
        tp->GetColor(ThemeProperties::COLOR_TOOLBAR));
    canvas->DrawLine(p2 + gfx::Vector2d(0 , height_tabstrip), p4 + gfx::Vector2d(0 , height_client),
        tp->GetColor(ThemeProperties::COLOR_TOOLBAR));

    canvas->DrawLine(p2 + gfx::Vector2d(-2 , height_client), p4 + gfx::Vector2d(2 , 0), background_color);
    canvas->DrawLine(p2 + gfx::Vector2d(-1 , height_client), p4 + gfx::Vector2d(1 , 0), background_color);
    canvas->DrawLine(p2 + gfx::Vector2d(0 , height_client), p4 + gfx::Vector2d(0 , 0), background_color);

    // 下
    canvas->DrawLine(p3 + gfx::Vector2d(0 , -1), p4 + gfx::Vector2d(0 , -1), background_color);
    canvas->DrawLine(p3 + gfx::Vector2d(0 , -2), p4 + gfx::Vector2d(0 , -2), background_color);
    canvas->DrawLine(p3, p4, background_color);
    }
  }
}

// BrowserNonClientFrameView:
bool OpaqueBrowserFrameView::ShouldPaintAsThemed() const {
  // Theme app and popup windows if |platform_observer_| wants it.
  return (browser_view()->IsBrowserTypeNormal() ||
         platform_observer_->IsUsingSystemTheme()) &&
         !browser_view()->browser()->is_simple_web_vedio();
}

void OpaqueBrowserFrameView::UpdateProfileIcons() {
  if (browser_view()->IsRegularOrGuestSession())
    profile_switcher_.Update(AvatarButtonStyle::THEMED);
  else
    UpdateProfileIndicatorIcon();
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameView, private:

views::ImageButton* OpaqueBrowserFrameView::InitWindowCaptionButton(
    int normal_image_id,
    int hot_image_id,
    int pushed_image_id,
    int mask_image_id,
    int accessibility_string_id,
    ViewID view_id) {
  views::ImageButton* button = new views::ImageButton(this);
  const ui::ThemeProvider* tp = frame()->GetThemeProvider();
  button->SetImage(views::CustomButton::STATE_NORMAL,
                   tp->GetImageSkiaNamed(normal_image_id));
  button->SetImage(views::CustomButton::STATE_HOVERED,
                   tp->GetImageSkiaNamed(hot_image_id));
  button->SetImage(views::CustomButton::STATE_PRESSED,
                   tp->GetImageSkiaNamed(pushed_image_id));
  if (false && browser_view()->IsBrowserTypeNormal()) {
    button->SetBackground(
        tp->GetColor(ThemeProperties::COLOR_BUTTON_BACKGROUND),
        tp->GetImageSkiaNamed(IDR_THEME_WINDOW_CONTROL_BACKGROUND),
        tp->GetImageSkiaNamed(mask_image_id));
  }
  button->SetAccessibleName(
      l10n_util::GetStringUTF16(accessibility_string_id));
  button->set_id(view_id);
  AddChildView(button);
  return button;
}

int OpaqueBrowserFrameView::FrameBorderThickness(bool restored) const {
  return layout_->FrameBorderThickness(restored);
}

int OpaqueBrowserFrameView::NonClientBorderThickness() const {
  return layout_->NonClientBorderThickness();
}

gfx::Rect OpaqueBrowserFrameView::IconBounds() const {
  return layout_->IconBounds();
}

bool OpaqueBrowserFrameView::ShouldShowWindowTitleBar() const {
  // Do not show the custom title bar if the system title bar option is enabled.
  if (!frame()->UseCustomFrame())
    return false;

  // Do not show caption buttons if the window manager is forcefully providing a
  // title bar (e.g., in Ubuntu Unity, if the window is maximized).
  if (!views::ViewsDelegate::GetInstance())
    return true;
  return !views::ViewsDelegate::GetInstance()->WindowManagerProvidesTitleBar(
      IsMaximized());
}

int OpaqueBrowserFrameView::GetTopAreaHeight() const {
  // The top area height in dp (only used when there's no frame image).
  // TODO(pkasting): investigate removing this constant. See crbug.com/590301
  const int kHeight = 30;
  const gfx::ImageSkia frame_image = GetFrameImage();
  int top_area_height = frame_image.isNull() ? kHeight : frame_image.height();
  if (browser_view()->IsTabStripVisible()) {
    top_area_height =
        std::max(top_area_height,
                 GetBoundsForTabStrip(browser_view()->tabstrip()).bottom());
  }
  return top_area_height;
}

void OpaqueBrowserFrameView::PaintRestoredFrameBorder(
    gfx::Canvas* canvas) const {
  frame_background_->set_frame_color(GetFrameColor());
  frame_background_->set_theme_image(GetFrameImage());
  frame_background_->set_theme_overlay_image(GetFrameOverlayImage());
  frame_background_->set_top_area_height(GetTopAreaHeight());

#ifdef huk //we don't want the side or corner
  const ui::ThemeProvider* tp = GetThemeProvider();
  frame_background_->SetSideImages(
      tp->GetImageSkiaNamed(IDR_WINDOW_LEFT_SIDE),
      tp->GetImageSkiaNamed(IDR_WINDOW_TOP_CENTER),
      tp->GetImageSkiaNamed(IDR_WINDOW_RIGHT_SIDE),
      tp->GetImageSkiaNamed(IDR_WINDOW_BOTTOM_CENTER));
  frame_background_->SetCornerImages(
      tp->GetImageSkiaNamed(IDR_WINDOW_TOP_LEFT_CORNER),
      tp->GetImageSkiaNamed(IDR_WINDOW_TOP_RIGHT_CORNER),
      tp->GetImageSkiaNamed(IDR_WINDOW_BOTTOM_LEFT_CORNER),
      tp->GetImageSkiaNamed(IDR_WINDOW_BOTTOM_RIGHT_CORNER));
#endif

  frame_background_->PaintRestored(canvas, this);

  // Note: When we don't have a toolbar, we need to draw some kind of bottom
  // edge here.  Because the App Window graphics we use for this have an
  // attached client edge and their sizing algorithm is a little involved, we do
  // all this in PaintRestoredClientEdge().
}

void OpaqueBrowserFrameView::PaintMaximizedFrameBorder(
    gfx::Canvas* canvas) const {
  frame_background_->set_frame_color(GetFrameColor());
  frame_background_->set_theme_image(GetFrameImage());
  frame_background_->set_theme_overlay_image(GetFrameOverlayImage());
  frame_background_->set_top_area_height(GetTopAreaHeight());
  frame_background_->set_maximized_top_inset(
      GetTopInset(true) - GetTopInset(false));
  frame_background_->PaintMaximized(canvas, this);
}

void OpaqueBrowserFrameView::PaintToolbarBackground(gfx::Canvas* canvas) const {
  // TODO(estade): can this be shared with OpaqueBrowserFrameView?
  gfx::Rect toolbar_bounds(browser_view()->GetToolbarBounds());
  if (toolbar_bounds.IsEmpty())
    return;
  gfx::Point toolbar_origin(toolbar_bounds.origin());
  ConvertPointToTarget(browser_view(), this, &toolbar_origin);
  toolbar_bounds.set_origin(toolbar_origin);

  const ui::ThemeProvider* tp = GetThemeProvider();
  const int x = toolbar_bounds.x();
  const int y = toolbar_bounds.y();
  const int w = toolbar_bounds.width();

  // Background.
  if (tp->HasCustomImage(IDR_THEME_TOOLBAR)) {
    canvas->TileImageInt(*tp->GetImageSkiaNamed(IDR_THEME_TOOLBAR),
                         x + GetThemeBackgroundXInset(),
                         y - GetTopInset(false) - GetLayoutInsets(TAB).top(), x,
                         y, w, toolbar_bounds.height());

    //paint transparent layer.
    gfx::Rect rect = toolbar_bounds;
    rect.set_height(toolbar_bounds.height() + browser_view()->GetBookmarkbarNoOverlapHeight());
    canvas->FillRect(rect, tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BLUR));
  } else {
    canvas->FillRect(toolbar_bounds,
                     tp->GetColor(ThemeProperties::COLOR_TOOLBAR));
  }

  // Top stroke.
  gfx::Rect separator_rect(x, y, w, 0);
  gfx::ScopedCanvas scoped_canvas(canvas);
  gfx::Rect tabstrip_bounds(GetBoundsForTabStrip(browser_view()->tabstrip()));
  tabstrip_bounds.set_x(GetMirroredXForRect(tabstrip_bounds));
  canvas->sk_canvas()->clipRect(gfx::RectToSkRect(tabstrip_bounds),
                                SkRegion::kDifference_Op);
  separator_rect.set_y(tabstrip_bounds.bottom());
  BrowserView::Paint1pxHorizontalLine(canvas, GetToolbarTopSeparatorColor(),
                                      separator_rect, true);

  // Toolbar/content separator.
  //[zhangyq] Don't show toolbar bottom separator if themed.
  if (!tp->HasCustomImage(IDR_THEME_TOOLBAR)) {
    BrowserView::Paint1pxHorizontalLine(
        canvas, tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BOTTOM_SEPARATOR),
        toolbar_bounds, true);
  }
}

void OpaqueBrowserFrameView::PaintClientEdge(gfx::Canvas* canvas) const {
  const bool tabstrip_visible = browser_view()->IsTabStripVisible();
  gfx::Rect client_bounds =
      layout_->CalculateClientAreaBounds(width(), height());
  const int x = client_bounds.x();
  int y = client_bounds.y();
  const int w = client_bounds.width();
  // If the toolbar isn't going to draw a top edge for us, draw one ourselves.
  if (!tabstrip_visible) {
    client_bounds.Inset(-kClientEdgeThickness, -1, -kClientEdgeThickness,
                        client_bounds.height());
    BrowserView::Paint1pxHorizontalLine(canvas, GetToolbarTopSeparatorColor(),
                                        client_bounds, true);
  }

  // In maximized mode, the only edge to draw is the top one, so we're done.
  if (layout_->IsTitleBarCondensed())
    return;

  const ui::ThemeProvider* tp = GetThemeProvider();
  const gfx::Rect toolbar_bounds(browser_view()->GetToolbarBounds());
  const bool incognito = browser_view()->IsIncognito();
  SkColor toolbar_color;
  if (tabstrip_visible) {
    toolbar_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR);

    // The client edge images start at the top of the toolbar.
    y += toolbar_bounds.y();
  } else {
    // Note that windows without tabstrips are never themed, so we always use
    // the default colors in this section.
    toolbar_color = ThemeProperties::GetDefaultColor(
        ThemeProperties::COLOR_TOOLBAR, incognito);
  }

  // Draw the client edges.
  const gfx::ImageSkia* const right_image =
      tp->GetImageSkiaNamed(IDR_CONTENT_RIGHT_SIDE);
  const int img_w = right_image->width();
  const int right = client_bounds.right();
  const int bottom = std::max(y, height() - NonClientBorderThickness());
  const int height = bottom - y;
  canvas->TileImageInt(*right_image, right, y, img_w, height);
  canvas->DrawImageInt(*tp->GetImageSkiaNamed(IDR_CONTENT_BOTTOM_RIGHT_CORNER),
                       right, bottom);
  const gfx::ImageSkia* const bottom_image =
      tp->GetImageSkiaNamed(IDR_CONTENT_BOTTOM_CENTER);
  canvas->TileImageInt(*bottom_image, x, bottom, w, bottom_image->height());
  canvas->DrawImageInt(*tp->GetImageSkiaNamed(IDR_CONTENT_BOTTOM_LEFT_CORNER),
                       x - img_w, bottom);
  canvas->TileImageInt(*tp->GetImageSkiaNamed(IDR_CONTENT_LEFT_SIDE), x - img_w,
                       y, img_w, height);
  FillClientEdgeRects(x, y, w, height, true, toolbar_color, canvas);

  // For popup windows, draw location bar sides.
  if (!tabstrip_visible && IsToolbarVisible()) {
    FillClientEdgeRects(
        x, y, w, toolbar_bounds.height(), false,
        LocationBarView::GetBorderColor(incognito), canvas);
  }
}

void OpaqueBrowserFrameView::FillClientEdgeRects(int x,
                                                 int y,
                                                 int w,
                                                 int h,
                                                 bool draw_bottom,
                                                 SkColor color,
                                                 gfx::Canvas* canvas) const {
  x -= kClientEdgeThickness;
  gfx::Rect side(x, y, kClientEdgeThickness, h);
  canvas->FillRect(side, color);
  if (draw_bottom) {
    canvas->FillRect(gfx::Rect(x, y + h, w + (2 * kClientEdgeThickness),
                               kClientEdgeThickness),
                     color);
  }
  side.Offset(w + kClientEdgeThickness, 0);
  canvas->FillRect(side, color);
}

void OpaqueBrowserFrameView::OnThemeChanged() {
  if (!IsSimpleWebVedio()) {
    const ui::ThemeProvider* tp = frame()->GetThemeProvider();
    bool dark = tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK) ? true:false;
    int id_theme_n = dark ? IDD_THEME_GALLERY_N_DARK : IDD_THEME_GALLERY_N;
    int id_theme_h = dark ? IDD_THEME_GALLERY_H_DARK : IDD_THEME_GALLERY_H;

    int id_min_n = dark ? IDD_FRAME_MIN_DARK : IDD_FRAME_MIN;
    int id_min_h = dark ? IDD_FRAME_MIN_H_DARK : IDD_FRAME_MIN_H;
    int id_min_p = dark ? IDD_FRAME_MIN_P_DARK : IDD_FRAME_MIN_P;

    int id_max_n = dark ? IDD_FRAME_MAX_DARK : IDD_FRAME_MAX;
    int id_max_h = dark ? IDD_FRAME_MAX_H_DARK : IDD_FRAME_MAX_H_DARK;
    int id_max_p = dark ? IDD_FRAME_MAX_P_DARK : IDD_FRAME_MAX_P_DARK;

    int id_restore_n = dark ? IDD_FRAME_RESTORE_DARK : IDD_FRAME_RESTORE;
    int id_restore_h = dark ? IDD_FRAME_RESTORE_H_DARK : IDD_FRAME_RESTORE_H;
    int id_restore_p = dark ? IDD_FRAME_RESTORE_P_DARK : IDD_FRAME_RESTORE_P;

    int id_close_n = dark ? IDD_FRAME_CLOSE_DARK : IDD_FRAME_CLOSE;
    int id_close_h = dark ? IDD_FRAME_CLOSE_H_DARK : IDD_FRAME_CLOSE_H;
    int id_close_p = dark ? IDD_FRAME_CLOSE_P_DARK : IDD_FRAME_CLOSE_P;

    if(browser_view_->browser()->is_type_tabbed()) {
      theme_button_->SetImage(views::CustomButton::STATE_NORMAL,
                     tp->GetImageSkiaNamed(id_theme_n));
      theme_button_->SetImage(views::CustomButton::STATE_HOVERED,
                     tp->GetImageSkiaNamed(id_theme_h));
      theme_button_->SetImage(views::CustomButton::STATE_PRESSED,
                     tp->GetImageSkiaNamed(id_theme_h));
    }

    minimize_button_->SetImage(views::CustomButton::STATE_NORMAL,
                   tp->GetImageSkiaNamed(id_min_n));
    minimize_button_->SetImage(views::CustomButton::STATE_HOVERED,
                   tp->GetImageSkiaNamed(id_min_h));
    minimize_button_->SetImage(views::CustomButton::STATE_PRESSED,
                   tp->GetImageSkiaNamed(id_min_p));

    maximize_button_->SetImage(views::CustomButton::STATE_NORMAL,
                   tp->GetImageSkiaNamed(id_max_n));
    maximize_button_->SetImage(views::CustomButton::STATE_HOVERED,
                   tp->GetImageSkiaNamed(id_max_h));
    maximize_button_->SetImage(views::CustomButton::STATE_PRESSED,
                   tp->GetImageSkiaNamed(id_max_p));

    restore_button_->SetImage(views::CustomButton::STATE_NORMAL,
                   tp->GetImageSkiaNamed(id_restore_n));
    restore_button_->SetImage(views::CustomButton::STATE_HOVERED,
                   tp->GetImageSkiaNamed(id_restore_h));
    restore_button_->SetImage(views::CustomButton::STATE_PRESSED,
                   tp->GetImageSkiaNamed(id_restore_p));

    close_button_->SetImage(views::CustomButton::STATE_NORMAL,
                   tp->GetImageSkiaNamed(id_close_n));
    close_button_->SetImage(views::CustomButton::STATE_HOVERED,
                   tp->GetImageSkiaNamed(id_close_h));
    close_button_->SetImage(views::CustomButton::STATE_PRESSED,
                   tp->GetImageSkiaNamed(id_close_p));
  }

  views::View::OnThemeChanged();
}
