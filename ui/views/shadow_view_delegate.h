#ifndef UI_VIEWS_SHADOW_VIEW_DELEGATE_H_
#define UI_VIEWS_SHADOW_VIEW_DELEGATE_H_

#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/geometry/point.h"

namespace ui{
  class MouseEvent;
}

namespace gfx{
  class Rect;
}

namespace views {

class VIEWS_EXPORT ShadowViewDelegate{

public:
  virtual ~ShadowViewDelegate(){}
  virtual bool HitTestRect(const gfx::Rect& rect) const = 0;
  virtual bool OnMousePressed(const ui::MouseEvent& event) = 0;
  virtual void OnMouseExited(const ui::MouseEvent& event) = 0;
  virtual void OnMouseMoved(const ui::MouseEvent& event) = 0;
  virtual gfx::NativeCursor GetCursor(const ui::MouseEvent& event) = 0;
  virtual bool resizable() const = 0;
};
}  // namespace views


#endif //UI_VIEWS_SHADOW_VIEW_DELEGATE_H_
