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

#ifndef UI_VIEWS_SHADOW_VIEW_H_
#define UI_VIEWS_SHADOW_VIEW_H_

#include "base/compiler_specific.h"
#include "base/timer/timer.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/masked_targeter_delegate.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/non_client_view.h"

namespace gfx {
  class ImageSkia;
}

namespace views {
  class Widget;
  class ShadowViewDelegate;

class  VIEWS_EXPORT ShadowViewBorder : public Border {
public:
  enum ShadowType {
    SHADOW    = 0,
    MESSAGE_BOX_SHADOW =1,
    LOCATIONBAR_SHADOW = 2,
    SHADOW_TYPE_COUNT,
  };

  ShadowViewBorder();

  void set_shadow_type(ShadowType type) { shadow_type_ = type;};
  ShadowType shadow_type() const { return shadow_type_;}
  void set_background_color(SkColor color) { background_color_ = color; }
  SkColor background_color() const { return background_color_; }
  void set_client_bounds(const gfx::Rect& bounds) { client_bounds_ = bounds; }
  const gfx::Rect& client_bounds() const { return client_bounds_; }

  const gfx::Rect coincident_part() const;

  //Border overrides:
   void Paint(const View& view, gfx::Canvas* canvas) override;
   gfx::Insets GetInsets() const override;
   gfx::Size GetMinimumSize() const override;

  int GetBorderCornerRadius() const;

protected:
   ~ShadowViewBorder() override;

private:
  struct BorderImages;

  static BorderImages* GetBorderImages(ShadowType type);
  static struct BorderImages* border_images_[SHADOW_TYPE_COUNT];

  SkColor background_color_;
  gfx::Rect client_bounds_;

  ShadowType  shadow_type_;

  DISALLOW_COPY_AND_ASSIGN(ShadowViewBorder);
};

class VIEWS_EXPORT ShadowViewFrame : public NonClientFrameView {
public:

  ShadowViewFrame(ShadowViewBorder* border, ShadowViewBorder::ShadowType type, ShadowViewDelegate* delegate);
   ~ShadowViewFrame() override;

  // NonClientFrameView overrides:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
  const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size,
                                             gfx::Path* window_mask) override;
  void ResetWindowControls() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;
  bool DoesIntersectRect(const View* target, const gfx::Rect& rect) const override;

  // Overridden from View:
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  gfx::NativeCursor GetCursor(const ui::MouseEvent& event) override;
  gfx::Size GetPreferredSize() const override;
  View* GetTooltipHandlerForPoint(const gfx::Point& point) override;

  void set_client_widget(Widget* client_widget) ;

  ShadowViewBorder::ShadowType type() const;

private:
  // Widget* frame_;

  ShadowViewBorder* shadow_border_;

  Widget* client_widget_;

  ShadowViewBorder::ShadowType type_;

  ShadowViewDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ShadowViewFrame);
};

class  VIEWS_EXPORT ShadowViewFrameBackground : public Background {
 public:
  explicit ShadowViewFrameBackground(ShadowViewBorder* border) : border_(border) {}

  // Overridden from Background:
  void Paint(gfx::Canvas* canvas, View* view) const override;

 private:
  ShadowViewBorder* border_;

  DISALLOW_COPY_AND_ASSIGN(ShadowViewFrameBackground);
};

class VIEWS_EXPORT ShadowView : public WidgetDelegate,
                                public WidgetObserver {

public:

  ShadowView(Widget* client_widget,
             ShadowViewDelegate* delegate,
             ShadowViewBorder::ShadowType type = ShadowViewBorder::SHADOW,
             int show_delay = 0,
             bool need_animation = false);
   ~ShadowView() override;

  Widget* client_widget() const;
  ShadowViewFrame* GetFrameView();;
  void set_show_delay(int delay);

  Widget* widget() { return widget_; }

private:

  // WidgetDelegate overrides:
   bool CanActivate() const override;
   Widget* GetWidget() override;
   const Widget* GetWidget() const override;
   NonClientFrameView* CreateNonClientFrameView(Widget* widget) override ;

  // WidgetObserver overrides:
   void OnWidgetVisibilityChanged(Widget* widget, bool visible) override;
   void OnWidgetBoundsChanged(Widget* widget, const gfx::Rect& new_bounds) override;
   void OnWidgetClosing(Widget* widget) override;
   void OnWidgetDestroyed(Widget* widget) override;
   void OnWidgetCreated(Widget* widget) override;

  void UpdateSize(const gfx::Rect& new_size);
private:
  void StartShow();
  void StartShowTimer();
  void StopShow();
  void CheckShadowShowCallBack();

private:

  Widget* client_widget_;
  Widget* widget_;
  base::Timer show_timer_;
  ShadowViewBorder::ShadowType type_;
  std::unique_ptr<ShadowViewDelegate> delegate_;
  int show_delay_;
  base::WeakPtrFactory<ShadowView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShadowView);
};

}  // namespace views


#endif //UI_VIEWS_SHADOW_VIEW_H_
