// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#include "ui/views/window/dialog_delegate.h"

#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/default_style.h"
#include "ui/base/hit_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/canvas.h"
#include "ui/resources/grit/ui_resources_nfs.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
//#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/shadow_view.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/dialog_client_view.h"

#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "ui/base/win/shell.h"
#include "base/win/windows_version.h"
#endif

namespace views {

////////////////////////////////////////////////////////////////////////////////
// DialogDelegate:

DialogDelegate::DialogDelegate() : supports_custom_frame_(true) {}

DialogDelegate::~DialogDelegate() {}

// static
Widget* DialogDelegate::CreateDialogWidget(WidgetDelegate* delegate,
                                           gfx::NativeWindow context,
                                           gfx::NativeView parent) {
  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params =
      GetDialogWidgetInitParams(delegate, context, parent, gfx::Rect());
  widget->Init(params);
  return widget;
}

// static
Widget::InitParams DialogDelegate::GetDialogWidgetInitParams(
    WidgetDelegate* delegate,
    gfx::NativeWindow context,
    gfx::NativeView parent,
    const gfx::Rect& bounds) {
  views::Widget::InitParams params;
  params.delegate = delegate;
  params.bounds = bounds;
  DialogDelegate* dialog = delegate->AsDialogDelegate();

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  // The new style doesn't support unparented dialogs on Linux desktop.
  if (dialog)
    dialog->supports_custom_frame_ &= parent != NULL;
#elif defined(OS_WIN)
  if (base::win::GetVersion() >= base::win::VERSION_WIN10)
    params.force_software_compositing = true;
  // The new style doesn't support unparented dialogs on Windows Classic themes.
  if (dialog)
    dialog->supports_custom_frame_ &= parent != NULL;
#endif

  if (!dialog || dialog->ShouldUseCustomFrame()) {

   // printf("%s\n", );
    params.opacity = Widget::InitParams::TRANSLUCENT_WINDOW;
    params.remove_standard_frame = true;
#if !defined(OS_MACOSX)
    // Except on Mac, the bubble frame includes its own shadow; remove any
    // native shadowing. On Mac, the window server provides the shadow.
    params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_NONE;
#endif
  }
  params.context = context;
  params.parent = parent;
#if !defined(OS_MACOSX)
  // Web-modal (ui::MODAL_TYPE_CHILD) dialogs with parents are marked as child
  // widgets to prevent top-level window behavior (independent movement, etc).
  // On Mac, however, the parent may be a native window (not a views::Widget),
  // and so the dialog must be considered top-level to gain focus and input
  // method behaviors.
  params.child = parent && (delegate->GetModalType() == ui::MODAL_TYPE_CHILD);
#endif
  return params;
}

View* DialogDelegate::CreateExtraView() {
  return NULL;
}

bool DialogDelegate::GetExtraViewPadding(int* padding) {
  return false;
}

View* DialogDelegate::CreateFootnoteView() {
  return NULL;
}

bool DialogDelegate::Cancel() {
  return true;
}

bool DialogDelegate::Accept() {
  return true;
}

bool DialogDelegate::Close() {
  int buttons = GetDialogButtons();
  if ((buttons & ui::DIALOG_BUTTON_CANCEL) ||
      (buttons == ui::DIALOG_BUTTON_NONE)) {
    return Cancel();
  }
  return Accept();
}

void DialogDelegate::UpdateButton(LabelButton* button, ui::DialogButton type) {
  button->SetText(GetDialogButtonLabel(type));
  button->SetEnabled(IsDialogButtonEnabled(type));
  bool is_default = type == GetDefaultDialogButton();
  if (!PlatformStyle::kDialogDefaultButtonCanBeCancel &&
      type == ui::DIALOG_BUTTON_CANCEL) {
    is_default = false;
  }
  button->SetIsDefault(is_default);
}

int DialogDelegate::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
}

int DialogDelegate::GetDefaultDialogButton() const {
  if (GetDialogButtons() & ui::DIALOG_BUTTON_OK)
    return ui::DIALOG_BUTTON_OK;
  if (GetDialogButtons() & ui::DIALOG_BUTTON_CANCEL)
    return ui::DIALOG_BUTTON_CANCEL;
  return ui::DIALOG_BUTTON_NONE;
}

bool DialogDelegate::ShouldDefaultButtonBeBlue() const {
  // return false;
  return true;
}

base::string16 DialogDelegate::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return l10n_util::GetStringUTF16(IDS_APP_OK);
  if (button == ui::DIALOG_BUTTON_CANCEL) {
    if (GetDialogButtons() & ui::DIALOG_BUTTON_OK)
      return l10n_util::GetStringUTF16(IDS_APP_CANCEL);
    return l10n_util::GetStringUTF16(IDS_APP_CLOSE);
  }
  NOTREACHED();
  return base::string16();
}

bool DialogDelegate::IsDialogButtonEnabled(ui::DialogButton button) const {
  return true;
}

View* DialogDelegate::GetInitiallyFocusedView() {
  // Focus the default button if any.
  const DialogClientView* dcv = GetDialogClientView();
  int default_button = GetDefaultDialogButton();
  if (default_button == ui::DIALOG_BUTTON_NONE)
    return NULL;

  if ((default_button & GetDialogButtons()) == 0) {
    // The default button is a button we don't have.
    NOTREACHED();
    return NULL;
  }

  if (default_button & ui::DIALOG_BUTTON_OK)
    return dcv->ok_button();
  if (default_button & ui::DIALOG_BUTTON_CANCEL)
    return dcv->cancel_button();
  return NULL;
}

DialogDelegate* DialogDelegate::AsDialogDelegate() {
  return this;
}

ClientView* DialogDelegate::CreateClientView(Widget* widget) {
  return new DialogClientView(widget, GetContentsView());
}

NonClientFrameView* DialogDelegate::CreateNonClientFrameView(Widget* widget) {
  if (ShouldUseCustomFrame())
    return CreateDialogFrameView(widget);
  return WidgetDelegate::CreateNonClientFrameView(widget);
}

// static
NonClientFrameView* DialogDelegate::CreateDialogFrameView(Widget* widget) {
  BubbleFrameView* frame =
      new BubbleFrameView(gfx::Insets(kPanelVertMargin, kButtonHEdgeMarginNew,
                                      0, kButtonHEdgeMarginNew),
                          gfx::Insets());
  const BubbleBorder::Shadow kShadow = BubbleBorder::NO_SHADOW;
  std::unique_ptr<BubbleBorder> border(
      new BubbleBorder(BubbleBorder::FLOAT, kShadow, gfx::kPlaceholderColor));
  border->set_use_theme_background_color(true);
  frame->SetBubbleBorder(std::move(border));
  DialogDelegate* delegate = widget->widget_delegate()->AsDialogDelegate();
  if (delegate)
    frame->SetFootnoteView(delegate->CreateFootnoteView());
  return frame;
}

bool DialogDelegate::ShouldUseCustomFrame() const {
  return supports_custom_frame_;
}

const DialogClientView* DialogDelegate::GetDialogClientView() const {
  return GetWidget()->client_view()->AsDialogClientView();
}

DialogClientView* DialogDelegate::GetDialogClientView() {
  return GetWidget()->client_view()->AsDialogClientView();
}

int DialogDelegate::NonClientHitTest(const gfx::Point& point) {
  return HTNOWHERE;
}

ui::AXRole DialogDelegate::GetAccessibleWindowRole() const {
  return ui::AX_ROLE_DIALOG;
}

////////////////////////////////////////////////////////////////////////////////
// DialogDelegateView:

DialogDelegateView::DialogDelegateView() {
  // A WidgetDelegate should be deleted on DeleteDelegate.
  set_owned_by_client();
}

DialogDelegateView::~DialogDelegateView() {}

void DialogDelegateView::DeleteDelegate() {
  delete this;
}

Widget* DialogDelegateView::GetWidget() {
  return View::GetWidget();
}

const Widget* DialogDelegateView::GetWidget() const {
  return View::GetWidget();
}

View* DialogDelegateView::GetContentsView() {
  return this;
}

void DialogDelegateView::GetAccessibleState(ui::AXViewState* state) {
  state->name = GetWindowTitle();
  state->role = ui::AX_ROLE_DIALOG;
}

void DialogDelegateView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this && GetWidget())
    NotifyAccessibilityEvent(ui::AX_EVENT_ALERT, true);
}

NfsDialogDelegate::NfsDialogDelegate()
  : dialog_client_view_(nullptr) {
}

NfsDialogDelegate::~NfsDialogDelegate() {
}

// static
Widget* NfsDialogDelegate::CreateNfsDialogWidget(WidgetDelegate* delegate,
                                           gfx::NativeWindow context,
                                           gfx::NativeView parent) {
  return CreateNfsDialogWidgetWithBounds(delegate, context, parent, gfx::Rect());
}

// static
Widget* NfsDialogDelegate::CreateNfsDialogWidgetWithBounds(WidgetDelegate* delegate,
                                                     gfx::NativeWindow context,
                                                     gfx::NativeView parent,
                                                     const gfx::Rect& bounds) {
  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params;
  params.delegate = delegate;
  params.bounds = bounds;
  NfsDialogDelegate* dialog = static_cast<NfsDialogDelegate*>(delegate->AsDialogDelegate());

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  // The new style doesn't support unparented dialogs on Linux desktop.
  if (dialog)
    dialog->supports_custom_frame_ &= parent != NULL;
#elif defined(OS_WIN)
  // The new style doesn't support unparented dialogs on Windows Classic themes.
  if (dialog)
    #if !defined(OS_WIN)
    dialog->supports_new_style_ &= parent != NULL;
  #endif
#endif

  if (!dialog || dialog->ShouldUseCustomFrame()) {
    params.remove_standard_frame = true;
    //remove title bar and border in custom frame view.
    params.remove_custom_frame = true;
    params.type = Widget::InitParams::TYPE_WINDOW;
  }

  params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_NONE;

  params.context = context;
  params.parent = parent;
#if !defined(OS_MACOSX)
  // Web-modal (ui::MODAL_TYPE_CHILD) dialogs with parents are marked as child
  // widgets to prevent top-level window behavior (independent movement, etc).
  // On Mac, however, the parent may be a native window (not a views::Widget),
  // and so the dialog must be considered top-level to gain focus and input
  // method behaviors.
   params.child = parent && (delegate->GetModalType() == ui::MODAL_TYPE_CHILD);
#endif

  widget->Init(params);

#if defined(OS_WIN)
  if (params.remove_custom_frame) {
    aura::WindowTreeHost* host = widget->GetNativeWindow()->GetHost();
    if (host) {
      HWND hwnd = host->GetAcceleratedWidget();
      ::SetWindowLong(hwnd, GWL_EXSTYLE, 0);
    }
  }
#endif

 new views::ShadowView(widget, NULL, views::ShadowViewBorder::MESSAGE_BOX_SHADOW, 0, true);
  //Set contents view's background.
  dialog->dialog_client_view_->set_background(
                    views::Background::CreateSolidBackground(
                    widget->GetNativeTheme()->GetSystemColor(
                    ui::NativeTheme::kColorId_DialogBackground)));
  return widget;
}

View* NfsDialogDelegate::GetInitiallyFocusedView() {
  // Focus the default button if any.
  const DialogClientView* dcv = static_cast<DialogClientView*>(dialog_client_view_);
  int default_button = GetDefaultDialogButton();
  if (default_button == ui::DIALOG_BUTTON_NONE)
    return NULL;

  if ((default_button & GetDialogButtons()) == 0) {
    // The default button is a button we don't have.
    NOTREACHED();
    return NULL;
  }

  if (default_button & ui::DIALOG_BUTTON_OK)
    return dcv->ok_button();
  if (default_button & ui::DIALOG_BUTTON_CANCEL)
    return dcv->cancel_button();
  return NULL;
}

DialogDelegate* NfsDialogDelegate::AsDialogDelegate() {
  return this;
}

ClientView* NfsDialogDelegate::CreateClientView(Widget* widget) {
  if (!dialog_client_view_) {
    dialog_client_view_ = new DialogClientView(GetWidget(), GetContentsView());
  }
  return static_cast<ClientView*>(dialog_client_view_);
}

NonClientFrameView* NfsDialogDelegate::CreateNonClientFrameView(Widget* widget) {
  return WidgetDelegate::CreateNonClientFrameView(widget);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static
const char TopContentsView::kViewClassName[] =
    "ui/views/Dialog/TopContentsView";

const SkColor kTitle_text_color = SkColorSetRGB(0x99, 0x99, 0x99);
const SkColor kTitle_bar_color = SkColorSetRGB(0xe8, 0xe8, 0xe8);

//const int kContents_view_width = 400;
const int kTitle_bar_height = 30;
const int kTitle_bar_horizontal_padding = 12;
const int kClose_image_width = 40;
const int kClose_image_height = 30;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ContentsView, public:
TopContentsView::TopContentsView(
      views::View* contents_view,
      DialogDelegate* delegate)
    : title_(nullptr),
      close_(nullptr),
      contens_view_(contents_view),
      delegate_(delegate),
      title_bar_color_(kTitle_bar_color),
      dragable_everywhere_(false),
      dragable_nowhere_(false) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  title_ = new views::Label(base::string16(), rb.GetFontListWithDelta(ui::kTitleFontSizeDelta));
  title_->set_collapse_when_hidden(true);
  title_->SetVisible(true);
  title_->SetMultiLine(true);
  title_->SetEnabledColor(kTitle_text_color);
  title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(title_);

  close_ = CreateCloseButton(this);
  close_->SetVisible(true);
  AddChildView(close_);

  AddChildView(contens_view_);
}

TopContentsView::~TopContentsView() {
}

// static
views::LabelButton* TopContentsView::CreateCloseButton(views::ButtonListener* listener) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  views::LabelButton* close = new views::LabelButton(listener, base::string16());
  close->SetImage(views::CustomButton::STATE_NORMAL,
                  *rb.GetImageNamed(IDD_ClOSE_DIALOG).ToImageSkia());
  close->SetImage(views::CustomButton::STATE_HOVERED,
                  *rb.GetImageNamed(IDD_ClOSE_DIALOG_H).ToImageSkia());
  close->SetImage(views::CustomButton::STATE_PRESSED,
                  *rb.GetImageNamed(IDD_ClOSE_DIALOG_P).ToImageSkia());
  close->SetBorder(nullptr);
  close->SetSize(close->GetPreferredSize());
#if !defined(OS_WIN)
  // Windows will automatically create a tooltip for the close button based on
  // the HTCLOSE result from NonClientHitTest().
  close->SetTooltipText(l10n_util::GetStringUTF16(IDS_APP_CLOSE));
#endif
  return close;
}

int TopContentsView::NonClientHitTest(const gfx::Point& point) {
  if (!bounds().Contains(point))
    return HTNOWHERE;
  if (close_->visible() && close_->GetMirroredBounds().Contains(point))
    return HTCLOSE;
  if (dragable_nowhere_)
    return HTNOWHERE;
  if (dragable_everywhere_ || point.y() < title_->bounds().bottom())
    return HTCAPTION;

  return HTNOWHERE;
}

bool TopContentsView::CanClose() {
  return true;
}

void TopContentsView::WidgetClosing() {
}

///////////////////////////////////////////////////////////////////////////////
// TopContentsView, View overrides:
void TopContentsView::OnPaint(gfx::Canvas* canvas) {
  canvas->FillRect(gfx::Rect(0, 0, contens_view_->GetPreferredSize().width(),
                  kTitle_bar_height), title_bar_color_);
  title_->SetText(delegate_->GetWindowTitle());

  views::View::OnPaint(canvas);
}

gfx::Size TopContentsView::GetPreferredSize() const {
  gfx::Size size = contens_view_->GetPreferredSize();
  return gfx::Size(size.width(), kTitle_bar_height + size.height());
}

void TopContentsView::Layout() {
    gfx::Size size = contens_view_->GetPreferredSize();
    title_->SetBounds(kTitle_bar_horizontal_padding, 0,
                                        size.width(), kTitle_bar_height);
    close_->SetBounds(size.width() - 1 - kClose_image_width,
                                        (kTitle_bar_height - kClose_image_height) / 2,
                                        kClose_image_width, kClose_image_height);
    contens_view_->SetBounds(0, kTitle_bar_height, size.width(), size.height());
}

const char* TopContentsView::GetClassName() const {
  return kViewClassName;
}

void TopContentsView::ButtonPressed(views::Button* sender, const ui::Event& event) {
  if (sender == close_) {
    GetWidget()->Close();
  }
}

void TopContentsView::SetTitleAlignment(gfx::HorizontalAlignment alignment) {
  title_->SetHorizontalAlignment(alignment);
}



///////////////////////////////////////////////////////////////////////////////
NfsDialogDelegateView::NfsDialogDelegateView(gfx::HorizontalAlignment align)
    : contents_view_(new TopContentsView(this, this)) {
      TopContentsView* contents_view = static_cast<TopContentsView*> (contents_view_);
      contents_view->SetTitleAlignment(align);
  // A WidgetDelegate should be deleted on DeleteDelegate.
  set_owned_by_client();
}

NfsDialogDelegateView::~NfsDialogDelegateView() {}

void NfsDialogDelegateView::DeleteDelegate() {
  delete this;
}

Widget* NfsDialogDelegateView::GetWidget() {
  return View::GetWidget();
}

const Widget* NfsDialogDelegateView::GetWidget() const {
  return View::GetWidget();
}

View* NfsDialogDelegateView::GetContentsView() {
  return contents_view_;
}

int NfsDialogDelegateView::NonClientHitTest(const gfx::Point& point) {
  return contents_view_->NonClientHitTest(point);
}

ui::ModalType NfsDialogDelegateView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}
}  // namespace views
