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

#include "ui/views/shadow_view.h"

#include <algorithm>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/win/windows_version.h"
#include "grit/ui_resources_nfs.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"

#include "ui/base/cursor/cursor.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/compositor_constants.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
//#include "ui/gfx/screen.h"  //zyq build modify
#include "ui/gfx/skia_util.h"
#include "ui/views/shadow_view_delegate.h"
#include "ui/views/widget/widget.h"

#include "ui/views/controls/native/native_view_host.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "content/public/browser/browser_thread.h"

namespace {

// Stroke size in pixels of borders in image assets.
//const int kBorderStrokeSize = 1;


// Border image resource ids.
const int kShadowImages[] = {
    IDD_SHADOW_L,
    IDD_SHADOW_TL,
    IDD_SHADOW_T,
    IDD_SHADOW_TR,
    IDD_SHADOW_R,
    IDD_SHADOW_BR,
    IDD_SHADOW_B,
    IDD_SHADOW_BL,
};

const int kMessageBoxShadowImages [] = {
    IDD_MESSAGE_BOX_SHADOW_L,
    IDD_MESSAGE_BOX_SHADOW_TL,
    IDD_MESSAGE_BOX_SHADOW_T,
    IDD_MESSAGE_BOX_SHADOW_TR,
    IDD_MESSAGE_BOX_SHADOW_R,
    IDD_MESSAGE_BOX_SHADOW_BR,
    IDD_MESSAGE_BOX_SHADOW_B,
    IDD_MESSAGE_BOX_SHADOW_BL,
};

const int kLocationBarShadowImages [] = {
    IDD_LOCATIONBAR_SHADOW_L,
    IDD_LOCATIONBAR_SHADOW_TL,
    IDD_LOCATIONBAR_SHADOW_T,
    IDD_LOCATIONBAR_SHADOW_TR,
    IDD_LOCATIONBAR_SHADOW_R,
    IDD_LOCATIONBAR_SHADOW_BR,
    IDD_LOCATIONBAR_SHADOW_B,
    IDD_LOCATIONBAR_SHADOW_BL,
};

// const int kSearchEnginShadowImage = IDR_OMNIBOX_SHADOW;
}  // namespace

namespace views {

struct ShadowViewBorder::BorderImages {
  BorderImages(const int image_ids[],
               size_t image_ids_size,
               int corner_radius = 0): corner_radius(corner_radius) {
    // Only two possible sizes of image ids array.
    DCHECK( image_ids_size == 8);

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    left = *rb.GetImageSkiaNamed(image_ids[0]);
    top_left = *rb.GetImageSkiaNamed(image_ids[1]);
    top = *rb.GetImageSkiaNamed(image_ids[2]);
    top_right = *rb.GetImageSkiaNamed(image_ids[3]);
    right = *rb.GetImageSkiaNamed(image_ids[4]);
    bottom_right = *rb.GetImageSkiaNamed(image_ids[5]);
    bottom = *rb.GetImageSkiaNamed(image_ids[6]);
    bottom_left = *rb.GetImageSkiaNamed(image_ids[7]);
  }

  BorderImages(int image_id, int size, int corner_radius = 0) :
      corner_radius(corner_radius) {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    gfx::ImageSkia whole_image = *rb.GetImageSkiaNamed(image_id);
    int inset = size;

    DCHECK_GE(whole_image.width(), 2 * inset);
    DCHECK_GE(whole_image.height(), 2 * inset);

    left = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(0, inset, inset,
        whole_image.height() - 2 * inset));
    top_left = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(0, 0, inset, inset));
    top = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(inset, 0,
        whole_image.width() - 2 * inset, inset));
    top_right = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(whole_image.width() - inset, 0,
        inset, inset));
    right =  gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(whole_image.width() - inset, inset,
        inset, whole_image.height() - 2 * inset));
    bottom_left = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(0, whole_image.height() - inset,
        inset, inset));
    bottom = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(inset, whole_image.height() - inset,
        whole_image.width() - 2 * inset, inset));
    bottom_right = gfx::ImageSkiaOperations::ExtractSubset(whole_image, gfx::Rect(whole_image.width() - inset,
        whole_image.height() - inset, inset, inset));
  }

  gfx::ImageSkia left;
  gfx::ImageSkia top_left;
  gfx::ImageSkia top;
  gfx::ImageSkia top_right;
  gfx::ImageSkia right;
  gfx::ImageSkia bottom_right;
  gfx::ImageSkia bottom;
  gfx::ImageSkia bottom_left;

  int corner_radius;
};

// static
struct ShadowViewBorder::BorderImages* ShadowViewBorder::border_images_[SHADOW_TYPE_COUNT] = { NULL };

ShadowViewBorder::ShadowViewBorder()
  : background_color_(SK_ColorWHITE),
    shadow_type_(SHADOW){
}

gfx::Insets ShadowViewBorder::GetInsets() const {
  BorderImages* images = GetBorderImages(shadow_type_);
  gfx::Insets insets;
  int top = images->top.height();
  int bottom = images->bottom.height();
  int left = images->left.width();
  int right = images->right.width();
  insets.Set(top, left, bottom, right);
  return insets;
}

gfx::Size ShadowViewBorder::GetMinimumSize() const {
  gfx::Insets insets = GetInsets();
  return gfx::Size(insets.width(), insets.height());
}

int ShadowViewBorder::GetBorderCornerRadius() const {
  return GetBorderImages(shadow_type_)->corner_radius;
}

// static
ShadowViewBorder::BorderImages*
ShadowViewBorder::GetBorderImages(ShadowType shadow) {
  struct BorderImages*& images = border_images_[shadow];
  if (images)
    return images;

  switch (shadow) {
    case SHADOW:
      images = new BorderImages(kShadowImages,
                                arraysize(kShadowImages),
                                3);
      break;
    case MESSAGE_BOX_SHADOW:
      images = new BorderImages(kMessageBoxShadowImages,
                                arraysize(kMessageBoxShadowImages),
                                0);
       break;

    case LOCATIONBAR_SHADOW:
      images = new BorderImages(kLocationBarShadowImages,
                                arraysize(kLocationBarShadowImages),
                                0);
       break;
    case SHADOW_TYPE_COUNT:
      NOTREACHED();
      break;
  }
  return images;
}

ShadowViewBorder::~ShadowViewBorder() {}

void ShadowViewBorder::Paint(const views::View& view, gfx::Canvas* canvas) {
  struct BorderImages* images = GetBorderImages(shadow_type_);
  int top_inset = images->top.height();
  int bottom_inset = images->bottom.height();
  int left_inset = images->left.width();
  int right_inset = images->right.width();
  int view_width = client_bounds().size().width();
  int view_height = client_bounds().size().height();
  int shadow_width = view_width - coincident_part().x() - coincident_part().width();
  int shadow_height = view_height - coincident_part().y() - coincident_part().height();

  canvas->TileImageInt(images->left, 0, top_inset, left_inset, shadow_height);
  canvas->TileImageInt(images->top_left, 0, 0, left_inset, top_inset);
  canvas->TileImageInt(images->top, left_inset, 0, shadow_width , top_inset);
  canvas->TileImageInt(images->top_right, left_inset + shadow_width, 0, right_inset, top_inset);
  canvas->TileImageInt(images->right, left_inset + shadow_width, top_inset, right_inset, shadow_height);
  canvas->TileImageInt(images->bottom_right, left_inset + shadow_width, top_inset + shadow_height, right_inset, bottom_inset);
  canvas->TileImageInt(images->bottom, left_inset, top_inset + shadow_height, shadow_width, bottom_inset);
  canvas->TileImageInt(images->bottom_left, 0, top_inset + shadow_height, left_inset, bottom_inset);
}

const gfx::Rect ShadowViewBorder::coincident_part() const {
  // 根据shadow type 得到阴影图片，得到重合区域 （设计图里面的白色部分, 每个阴影可能不一样)
  // 这里的rect 仅仅是为了使用方便，存储 左 上 右 下 四个方向的重合并大小。不代表真实的rect
  switch (shadow_type_) {
    case ShadowViewBorder::MESSAGE_BOX_SHADOW:
      return gfx::Rect(10, 13, 10, 5);

   case ShadowViewBorder::LOCATIONBAR_SHADOW:
      return gfx::Rect(6, 9, 6, 3);

    default:
      return gfx::Rect();
  }
}


void ShadowViewFrameBackground::Paint(gfx::Canvas* canvas, views::View* view) const {
  if (!border_->client_bounds().IsEmpty()) {
    SkRect client_rect(gfx::RectToSkRect(border_->client_bounds()));
    canvas->sk_canvas()->clipRect(client_rect, SkRegion::kDifference_Op);
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(border_->background_color());
  SkPath path;
  gfx::Rect bounds(border_->client_bounds());
  SkScalar radius = SkIntToScalar(border_->GetBorderCornerRadius());
  path.addRoundRect(gfx::RectToSkRect(bounds), radius, radius);
  canvas->DrawPath(path, paint);
}

ShadowViewFrame::ShadowViewFrame(ShadowViewBorder* border, ShadowViewBorder::ShadowType type, ShadowViewDelegate* delegate)
  : shadow_border_(border),
    type_(type),
    delegate_(delegate)  {
  SetBorder(std::unique_ptr<Border>(shadow_border_));
}

ShadowViewFrame::~ShadowViewFrame() {
}

gfx::Rect ShadowViewFrame::GetBoundsForClientView() const {
  gfx::Insets insets = shadow_border_->GetInsets();
  return gfx::Rect(insets.left(), insets.top(),
                   std::max(width() - insets.width(), 0),
                   std::max(height() - insets.height(), 0));
}

gfx::Rect ShadowViewFrame::GetWindowBoundsForClientBounds(const gfx::Rect& client_bounds) const {
  gfx::Insets insets = shadow_border_->GetInsets();
  gfx::Rect window_bounds = gfx::Rect(client_bounds.x() - insets.left(),
                                      client_bounds.y() - insets.top(),
                                      client_bounds.width() + insets.width(),
                                      client_bounds.height() + insets.height());
  return window_bounds;
}

int ShadowViewFrame::NonClientHitTest(const gfx::Point& point) {
  return GetWidget()->client_view()->NonClientHitTest(point);
}

bool ShadowViewFrame::OnMousePressed(const ui::MouseEvent& event) {
  if(delegate_)
    return delegate_->OnMousePressed(event);
  else
    return NonClientFrameView::OnMousePressed(event);
}

void ShadowViewFrame::OnMouseExited(const ui::MouseEvent& event) {
  if(delegate_)
    return delegate_->OnMouseExited(event);
  else
    return NonClientFrameView::OnMouseExited(event);
}

void ShadowViewFrame::OnMouseMoved(const ui::MouseEvent& event) {
  if(delegate_)
    return delegate_->OnMouseMoved(event);
  else
    return NonClientFrameView::OnMouseMoved(event);
}

gfx::NativeCursor ShadowViewFrame::GetCursor(const ui::MouseEvent& event) {
  if(delegate_)
    return delegate_->GetCursor(event);
  else
    return NonClientFrameView::GetCursor(event);
}

gfx::Size ShadowViewFrame::GetPreferredSize() const {
  gfx::Size client_size(GetWidget()->client_view()->GetPreferredSize());
  gfx::Insets margin = shadow_border_->GetInsets();
  client_size.Enlarge(margin.width(), margin.height());
  return client_size;
}

void ShadowViewFrame::set_client_widget(Widget* client_widget) {
  client_widget_ = client_widget;
}

ShadowViewBorder::ShadowType ShadowViewFrame::type() const {
  return type_;
}

void ShadowViewFrame::GetWindowMask(const gfx::Size& size,
                                    gfx::Path* window_mask) {

}
void ShadowViewFrame::ResetWindowControls() {

}

void ShadowViewFrame::UpdateWindowIcon() {

}

void ShadowViewFrame::ShadowViewFrame::UpdateWindowTitle() {
}

void ShadowViewFrame::SizeConstraintsChanged() {
}

bool ShadowViewFrame::DoesIntersectRect(const View* target, const gfx::Rect& rect) const {
  return true;
}

View* ShadowViewFrame::GetTooltipHandlerForPoint(const gfx::Point& point) { 
  return NULL; 
}

ShadowView::ShadowView(Widget* client_widget,
    ShadowViewDelegate* delegate,
    ShadowViewBorder::ShadowType type,
    int show_delay,
    bool need_animation)
    : client_widget_(client_widget),
      show_timer_(false, false),
      type_(type),
      delegate_(delegate),
      show_delay_(show_delay),
      weak_ptr_factory_(this) {
  DCHECK(client_widget_);
  client_widget_->AddObserver(this);

  widget_ = new Widget();
  Widget::InitParams params;
  params.delegate = this;

#if defined(OS_LINUX) 
  params.opacity = Widget::InitParams::TRANSLUCENT_WINDOW;
  params.activatable = delegate_ ? (delegate_->resizable() ? Widget::InitParams::ACTIVATABLE_YES : Widget::InitParams::ACTIVATABLE_NO) :
          Widget::InitParams::ACTIVATABLE_DEFAULT;
#endif

  params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_NONE;
  params.accept_events = delegate_ ? delegate_->resizable() : false;
  params.type = Widget::InitParams::TYPE_POPUP;
  params.parent = client_widget_->GetNativeView();
  widget_->set_focus_on_creation(false);
  widget_->Init(params);

  widget_->SetContentsView(CreateNonClientFrameView(widget_));

#if defined(OS_WIN)
  widget_->GetCompositor()->SetHostHasTransparentBackground(true);  
  HWND native_handle = widget_->GetNativeView()->GetHost()->GetAcceleratedWidget();
  ::SetProp(native_handle, kForceSoftwareCompositor, reinterpret_cast<HANDLE>(true));
  LONG style = ::GetWindowLong(native_handle, GWL_STYLE);
  style &= ~WS_SYSMENU;
  ::SetWindowLong(native_handle, GWL_STYLE, style);

  LONG ex_style = ::GetWindowLong(native_handle, GWL_EXSTYLE);
  ex_style |= WS_EX_TOOLWINDOW;
  ex_style |= WS_EX_NOACTIVATE;
  ex_style |= WS_EX_LAYERED;
  ex_style |= WS_EX_COMPOSITED;

  ::SetWindowLong(native_handle, GWL_EXSTYLE, ex_style);
#endif 

  if (!need_animation)  {
    widget_->SetVisibilityAnimationTransition(views::Widget::ANIMATE_NONE);    
  }

  UpdateSize(client_widget_->GetWindowBoundsInScreen());
  if(client_widget_->IsVisible()) {
    StartShowTimer();
  }
 // widget_->AddObserver(this);
}

ShadowView::~ShadowView() {
  if(client_widget_) {
    client_widget_->RemoveObserver(this);
  }
  // if (widget_)  {
  //  // widget_->RemoveObserver(this);
  // }
}

bool ShadowView::CanActivate() const {
  return false;
}

NonClientFrameView* ShadowView::CreateNonClientFrameView(Widget* widget){
  ShadowViewBorder* border = new ShadowViewBorder;
  border->set_shadow_type(type_);
  ShadowViewFrame* frame = new ShadowViewFrame(border, type_, delegate_.get());
  frame->set_background(new ShadowViewFrameBackground(border));
  frame->set_client_widget(client_widget_);
  return frame;
}

ShadowViewFrame* ShadowView::GetFrameView(){
  return static_cast<ShadowViewFrame*>(widget_->GetContentsView());
}

void ShadowView::StartShowTimer(){
  show_timer_.Start(FROM_HERE,
    base::TimeDelta::FromMilliseconds(show_delay_),
    base::Bind(&ShadowView::StartShow, weak_ptr_factory_.GetWeakPtr()));
}

void ShadowView::StartShow() {
  UpdateSize(client_widget_->GetWindowBoundsInScreen());
  widget_->Show();
}

void ShadowView::CheckShadowShowCallBack() {
  if (client_widget_->IsVisible()) {
    StartShowTimer();
  }
}

void ShadowView::OnWidgetCreated(Widget* widget) {
}

void ShadowView::OnWidgetVisibilityChanged(Widget* widget, bool visible){
  if (widget_ == widget) {
    return;
  }
  if(visible &&
    (!client_widget_->IsMinimized() && !client_widget_->IsMaximized() && !client_widget_->IsFullscreen()))
    StartShowTimer();
  else if(!client_widget_->IsMinimized()){
    widget_->Hide();
  }
}

void ShadowView::OnWidgetBoundsChanged(Widget* widget, const gfx::Rect& new_bounds){
  if (widget_ == widget) {
    return;
  }
  if(client_widget_->IsMaximized() || client_widget_->IsFullscreen() || client_widget_->IsMinimized()){
    widget_->Hide();
    return;
  } else {
    if(!widget_->IsVisible())
      StartShowTimer();
    else
      UpdateSize(new_bounds);
  }
}

void ShadowView::OnWidgetClosing(Widget* widget){
  widget_->Hide();
}

void ShadowView::OnWidgetDestroyed(Widget* widget) {
  if (widget == widget_)  {
    return;
  }
  // if (widget_)  {
  //   // widget_->RemoveObserver(this);
  // }
  delete this;
}

void ShadowView::UpdateSize(const gfx::Rect& new_size){
  ShadowViewFrame* frame_view = GetFrameView();
  gfx::Rect bounds = new_size;

  gfx::Rect frame_rect = frame_view->GetWindowBoundsForClientBounds(bounds);
  // 考虑到原本图片跟阴影图片重合的部分，即是设计师提供图片里面的白色部分
 
  ShadowViewBorder* border = static_cast<ShadowViewBorder*>(frame_view->border());
  gfx::Rect coincident_rect = border->coincident_part();
  frame_rect.Inset(coincident_rect.x(), coincident_rect.y(), coincident_rect.width(), coincident_rect.height());

  border->set_client_bounds(gfx::Rect(bounds.x() - frame_rect.x(),
      bounds.y() - frame_rect.y() ,
      bounds.width(), bounds.height()) );
  // NOTE: huk todo: double check  the magic number 8
  widget_->SetBounds(frame_rect);
}

Widget* ShadowView::client_widget() const {
  return client_widget_;
}

void ShadowView::set_show_delay(int delay) {
  show_delay_ = delay;
 }

 Widget* ShadowView::GetWidget()  {
   return widget_;
 }

 const Widget* ShadowView::GetWidget() const  {
   return widget_;
 }

}  // namespace views

