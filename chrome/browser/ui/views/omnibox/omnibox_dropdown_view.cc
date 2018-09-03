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

#include "chrome/browser/ui/views/omnibox/omnibox_dropdown_view.h"

#include <algorithm>
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_dropdown_entry_view.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"
#include "chrome/browser/ui/browser.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/history/core/browser/top_sites.h"
#include "grit/theme_resources.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/compositor/clip_recorder.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/path.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/shadow_view.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "chrome/browser/chrome_notification_types.h"
#include "ui/display/screen.h"

namespace {
  static int kSlideDuration = 250;
  const size_t kMaxEntryCountToShow = 10;
}

class OmniboxDropdownView::PopupWidget
    : public ThemeCopyingWidget,
      public base::SupportsWeakPtr<PopupWidget> {
 public:
  explicit PopupWidget(views::Widget* role_model)
      : ThemeCopyingWidget(role_model) {}
  ~PopupWidget() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PopupWidget);
};

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, public:

OmniboxDropdownView* OmniboxDropdownView::Create(
    const gfx::FontList& font_list,
    OmniboxView* omnibox_view,
    OmniboxEditModel* edit_model,
    LocationBarView* location_bar_view) {
  OmniboxDropdownView* view = NULL;
  view = new OmniboxDropdownView(
      font_list, omnibox_view, edit_model, location_bar_view);
  return view;
}

OmniboxDropdownView::OmniboxDropdownView(
    const gfx::FontList& font_list,
    OmniboxView* omnibox_view,
    OmniboxEditModel* edit_model,
    LocationBarView* location_bar_view)
    : model_(new OmniboxDropdownModel(location_bar_view->browser(), this, edit_model)),
      omnibox_view_(omnibox_view),
      location_bar_view_(location_bar_view),
      font_list_(font_list),
      ignore_mouse_drag_(false),
      start_margin_(0),
      end_margin_(0),
      size_animation_(this) {
  // The contents is owned by the LocationBarView.
  set_owned_by_client();

  model_->AddObserver(this);

  #if 0
  ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
  if (ui::MaterialDesignController::IsModeMaterial()) {
    top_shadow_ = rb->GetImageSkiaNamed(IDR_OMNIBOX_DROPDOWN_SHADOW_TOP);
    bottom_shadow_ = rb->GetImageSkiaNamed(IDR_OMNIBOX_DROPDOWN_SHADOW_BOTTOM);
  } else {
    bottom_shadow_ = rb->GetImageSkiaNamed(IDR_BUBBLE_B);
  }
  #else
  top_shadow_ = new gfx::ImageSkia();
  bottom_shadow_ = new gfx::ImageSkia();
  #endif

  size_animation_.SetSlideDuration(kSlideDuration);

  SetEventTargeter(
      std::unique_ptr<views::ViewTargeter>(new views::ViewTargeter(this)));
  set_notify_enter_exit_on_child(true);

  registrar_.Add(this, chrome::NOTIFICATION_MOUSE_PRESSED,
             content::NotificationService::AllSources());
}

OmniboxDropdownView::~OmniboxDropdownView() {
  model_->RemoveObserver(this);
  registrar_.Remove(this, chrome::NOTIFICATION_MOUSE_PRESSED,
             content::NotificationService::AllSources());
}

gfx::Rect OmniboxDropdownView::GetPopupBounds() const {
  if (!size_animation_.is_animating())
    return target_bounds_;

  gfx::Rect current_frame_bounds = start_bounds_;
  int total_height_delta = target_bounds_.height() - start_bounds_.height();
  // Round |current_height_delta| instead of truncating so we won't leave single
  // white pixels at the bottom of the popup as long when animating very small
  // height differences.
  int current_height_delta = static_cast<int>(
      size_animation_.GetCurrentValue() * total_height_delta - 0.5);
  current_frame_bounds.set_height(
      current_frame_bounds.height() + current_height_delta);
  return current_frame_bounds;
}

//OmniboxDropdownDataObserver:
void OmniboxDropdownView::OnOmniboxDropdownDataLoaded() {
  // printf("OmniboxDropdownView  loaded!!!!!!!!!!\n");
  if (child_count()) {
    RemoveAllChildViews(true);
  }
  AddChildViews();

  // In non-material mode, we want the popup to appear as if it's overlaying
  // the top of the page content, i.e., is flush against the client edge at the
  // bottom of the toolbar. However, if the bookmarks bar is attached, we want
  // to draw over it (so as not to push the results below it), but that means
  // the toolbar won't be drawing a client edge separating itself from the
  // popup. So we unconditionally overlap the toolbar by the thickness of the
  // client edge and draw our own edge (see OnPaint()), which fixes the
  // attached bookmark bar case without breaking the other case.
  int top_edge_overlap = views::NonClientFrameView::kClientEdgeThickness;
  if (ui::MaterialDesignController::IsModeMaterial()) {
    // In material mode, we cover the bookmark bar similarly, but instead of
    // appearing below the client edge, we want the popup to appear to overlay
    // the bottom of the toolbar. So instead of drawing a client edge atop the
    // popup, we shift the popup to completely cover the client edge, and then
    // draw an additional semitransparent shadow above that. So the total
    // overlap necessary is the client edge thickness plus the shadow height.
    top_edge_overlap += top_shadow_->height();
  }

  gfx::Point top_left_screen_coord;
  int width;
  location_bar_view_->GetOmniboxPopupPositioningInfo(
      &top_left_screen_coord, &width, &start_margin_,
      &end_margin_, top_edge_overlap);
  top_left_screen_coord.set_x(top_left_screen_coord.x() + start_margin_ + 2);
  top_left_screen_coord.set_y(top_left_screen_coord.y() - 10);
  width -= (start_margin_ + end_margin_ + 2);
  gfx::Rect new_target_bounds(top_left_screen_coord,
                              gfx::Size(width, CalculatePopupHeight()));

  // If we're animating and our target height changes, reset the animation.
  // NOTE: If we just reset blindly on _every_ update, then when the user types
  // rapidly we could get "stuck" trying repeatedly to animate shrinking by the
  // last few pixels to get to one visible result.
  target_bounds_ = new_target_bounds;
  size_animation_.Reset();

  if (popup_ == NULL) {
    views::Widget* popup_parent = location_bar_view_->GetWidget();

    // If the popup is currently closed, we need to create it.
    popup_ = (new PopupWidget(popup_parent))->AsWeakPtr();

    views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
#if defined(OS_WIN)
    // On Windows use the software compositor to ensure that we don't block
    // the UI thread blocking issue during command buffer creation. We can
    // revert this change once http://crbug.com/125248 is fixed.
    params.force_software_compositing = true;
#endif
    params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
    params.parent = popup_parent->GetNativeView();
    gfx::Rect initial_bounds = GetPopupBounds();
    initial_bounds.set_height(1);
    params.bounds = initial_bounds;
    params.context = popup_parent->GetNativeWindow();
    popup_->Init(params);
    // Third-party software such as DigitalPersona identity verification can
    // hook the underlying window creation methods and use SendMessage to
    // synchronously change focus/activation, resulting in the popup being
    // destroyed by the time control returns here.  Bail out in this case to
    // avoid a NULL dereference.
    if (!popup_.get())
      return;
    popup_->SetVisibilityAnimationTransition(views::Widget::ANIMATE_NONE);
    popup_->SetContentsView(this);
    popup_->StackAbove(omnibox_view_->GetRelativeWindowForPopup());
    if (!popup_.get()) {
      // For some IMEs GetRelativeWindowForPopup triggers the omnibox to lose
      // focus, thereby closing (and destroying) the popup.
      // TODO(sky): this won't be needed once we close the omnibox on input
      // window showing.
      return;
    }

    // add the shadow
    new views::ShadowView(static_cast<views::Widget*>(popup_.get()), NULL,
        views::ShadowViewBorder::LOCATIONBAR_SHADOW);

    popup_->ShowInactive();

    // Animate the popup shrinking, but don't animate growing larger since that
    // would make the popup feel less responsive.
    start_bounds_ = GetWidget()->GetWindowBoundsInScreen();
    size_animation_.Show();
  } else {
    popup_->SetBounds(GetPopupBounds());
  }

  Layout();
}

void OmniboxDropdownView::OnOmniboxDropdownDataChanged() {
  // printf("OmniboxDropdownView  OnOmniboxDropdownDataChanged\n");
  if (IsOpen()) {
    Invalidate();
  }
}

void OmniboxDropdownView::OnOmniboxDropdownEntryUpdate(
    const std::vector<GURL>& urls) {
  // DCHECK(index >= 0 && static_cast<int>(index) < child_count());
  if (IsOpen()) {
    for (const auto& url : urls) {
      UpdateEntry(url);
    }
  }
}

void OmniboxDropdownView::AddChildViews() {
  size_t count = EntryCountToShow();
  if (!count) {
    OmniboxDropdownDataEntry* empty_data = model_->GetEmptyEntry();
    if(GetItemByUrl(empty_data->url)) {
      return; //exist
    }
    OmniboxDropdownEntryView* item_view = new OmniboxDropdownEntryView(
              this, location_bar_view_, font_list_, true);
    item_view->SetVisible(true);
    item_view->SetEntryData(empty_data);
    AddChildViewAt(item_view, 0);
    return;
  }

  for (size_t i = 0; i < count; ++i) {
    OmniboxDropdownDataEntry* data = model_->DataEntry_at(i);
    OmniboxDropdownEntryView* item_view = CreateItemView(font_list_);
    item_view->SetVisible(true);
    item_view->SetEntryData(data);
    AddChildViewAt(item_view, static_cast<int>(i));
  }
}

void OmniboxDropdownView::LayoutChildren() {
  gfx::Rect contents_rect = GetContentsBounds();
  contents_rect.Inset(GetLayoutInsets(OMNIBOX_DROPDOWN));
  contents_rect.Inset(0, views::NonClientFrameView::kClientEdgeThickness, 0, 0);

  // In the non-material dropdown, the colored/clickable regions within the
  // dropdown are only as wide as the location bar. In the material version,
  // these are full width, and OmniboxDropdownEntryView instead insets the icons/text
  // inside to be aligned with the location bar.
  if (!ui::MaterialDesignController::IsModeMaterial())
    contents_rect.Inset(start_margin_, 0, end_margin_, 0);

  int top = contents_rect.y();
  size_t count = EntryCountToShow();
  count = count ? count : 1;
  for (size_t i = 0; i < count; ++i) {
    View* v = child_at(i);
    if (v->visible()) {
      v->SetBounds(contents_rect.x(), top, contents_rect.width(),
                   v->GetPreferredSize().height());
      top = v->bounds().bottom();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, OmniboxDropdownView overrides:

bool OmniboxDropdownView::IsOpen() const {
  return popup_ != NULL;
}

void OmniboxDropdownView::InvalidateLine(size_t line) {
  DCHECK(static_cast<int>(line) < child_count());
  OmniboxDropdownEntryView* item = Item_view_at(line);
  item->Invalidate();
}

void OmniboxDropdownView::OnLineSelected(size_t line) {
  Item_view_at(line)->OnSelected();
}

void OmniboxDropdownView::UpdatePopupAppearance() {
  if (omnibox_view_->IsImeShowingPopup()) {
    // If the IME is showing a popup window which may overlap
    // the omnibox popup window.  Close any existing popup.
    ClosePopup();
    return;
  }

  model_->Load();
}

void  OmniboxDropdownView::ClosePopup() {
    if (popup_ != NULL) {
      size_animation_.Stop();
      popup_->Close();
      popup_.reset();
      model_->Revert();
    }
}

gfx::Rect OmniboxDropdownView::GetTargetBounds() {
  return target_bounds_;
}

void OmniboxDropdownView::PaintUpdatesNow() {
  // TODO(beng): remove this from the interface.
}

void OmniboxDropdownView::OnDragCanceled() {
  ignore_mouse_drag_ = true;
}


////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, OmniboxDropdownEntryViewModel implementation:

bool OmniboxDropdownView::IsSelectedIndex(size_t index) const {
  return index == model_->selected_line();
}

bool OmniboxDropdownView::IsHoveredIndex(size_t index) const {
  return index == model_->hovered_line();
}

void OmniboxDropdownView::RemoveEntry(size_t line) {
  DCHECK(line >= 0);
  model_->SetHoveredLine(OmniboxDropdownModel::kNoMatch);
  RemoveChildView(Item_view_at(line));
  model_->RemoveEntry(line);
  if (EntryCountToShow() > line) {
    model_->SetHoveredLine(line);
  }
  Invalidate();
}

 size_t OmniboxDropdownView::GetIndexByUrl(const GURL& url) {
  for (int i = 0; i < child_count(); ++i) {
    OmniboxDropdownEntryView* entry = Item_view_at(i);
    if (url == entry->GetUrl()) {
      return i;
    }
  }
  return OmniboxDropdownModel::kNoMatch;
 }

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, AnimationDelegate implementation:

void OmniboxDropdownView::AnimationProgressed(
    const gfx::Animation* animation) {
  // We should only be running the animation when the popup is already visible.
  DCHECK(popup_ != NULL);
  popup_->SetBounds(GetPopupBounds());
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, views::View overrides:

void OmniboxDropdownView::Layout() {
  // Size our children to the available content area.
  LayoutChildren();

  // We need to manually schedule a paint here since we are a layered window and
  // won't implicitly require painting until we ask for one.
  SchedulePaint();
}

views::View* OmniboxDropdownView::GetTooltipHandlerForPoint(
    const gfx::Point& point) {
  return NULL;
}

bool OmniboxDropdownView::OnMousePressed(
    const ui::MouseEvent& event) {
  ignore_mouse_drag_ = false;  // See comment on |ignore_mouse_drag_| in header.
  if (event.IsLeftMouseButton() || event.IsMiddleMouseButton())
    UpdateLineEvent(event, event.IsLeftMouseButton());
  return true;
}

bool OmniboxDropdownView::OnMouseDragged(
    const ui::MouseEvent& event) {
  if (event.IsLeftMouseButton() || event.IsMiddleMouseButton())
    UpdateLineEvent(event, !ignore_mouse_drag_ && event.IsLeftMouseButton());
  return true;
}

void OmniboxDropdownView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (ignore_mouse_drag_) {
    OnMouseCaptureLost();
    return;
  }

  if (event.IsOnlyMiddleMouseButton() || event.IsOnlyLeftMouseButton()) {
    OpenSelectedLine(event, event.IsOnlyLeftMouseButton()
                                         ? WindowOpenDisposition::CURRENT_TAB :
                                            WindowOpenDisposition::NEW_BACKGROUND_TAB);
  }
}

void OmniboxDropdownView::OnMouseCaptureLost() {
  ignore_mouse_drag_ = false;
}

void OmniboxDropdownView::OnMouseMoved(
    const ui::MouseEvent& event) {
  model_->SetHoveredLine(GetIndexForPoint(event.location()));
}

void OmniboxDropdownView::OnMouseEntered(
    const ui::MouseEvent& event) {
  model_->SetHoveredLine(GetIndexForPoint(event.location()));
}

void OmniboxDropdownView::OnMouseExited(
    const ui::MouseEvent& event) {
  model_->SetHoveredLine(OmniboxDropdownModel::kNoMatch);
}

void OmniboxDropdownView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_TAP_DOWN:
    case ui::ET_GESTURE_SCROLL_BEGIN:
    case ui::ET_GESTURE_SCROLL_UPDATE:
      UpdateLineEvent(*event, true);
      break;
    case ui::ET_GESTURE_TAP:
    case ui::ET_GESTURE_SCROLL_END:
      OpenSelectedLine(*event, WindowOpenDisposition::CURRENT_TAB);
      break;
    default:
      return;
  }
  event->SetHandled();
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, protected:

int OmniboxDropdownView::CalculatePopupHeight() {
  DCHECK_GE(static_cast<size_t>(child_count()), EntryCountToShow());
  int popup_height = 0;
  size_t count = EntryCountToShow();
  count = count ? count : 1;
  for (size_t i = 0; i < count; ++i)
    popup_height += child_at(i)->GetPreferredSize().height();

   // Add enough space on the top and bottom so it looks like there is the same
  // amount of space between the text and the popup border as there is in the
  // interior between each row of text.
  return popup_height + GetLayoutInsets(OMNIBOX_DROPDOWN).height();// +
     //    g_top_shadow.Get().height() + g_bottom_shadow.Get().height();
}

OmniboxDropdownEntryView* OmniboxDropdownView::CreateItemView(
    const gfx::FontList& font_list) {
  return new OmniboxDropdownEntryView(this,
                                location_bar_view_, font_list);
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, views::View overrides, private:

const char* OmniboxDropdownView::GetClassName() const {
  return "OmniboxDropdownView";
}

void OmniboxDropdownView::OnPaint(gfx::Canvas* canvas) {
  /*
  // Top border.
  if (ui::MaterialDesignController::IsModeMaterial()) {
    canvas->TileImageInt(*top_shadow_, 0, 0, width(), top_shadow_->height());
  } else {
    canvas->FillRect(gfx::Rect(0, 0, width(),
                               views::NonClientFrameView::kClientEdgeThickness),
                     location_bar_view_->GetThemeProvider()->GetColor(
                         ThemeProperties::COLOR_TOOLBAR_BOTTOM_SEPARATOR));
  }

  // Bottom border.
  canvas->TileImageInt(*bottom_shadow_, 0, height() - bottom_shadow_->height(),
                       width(), bottom_shadow_->height());
  */
}

void OmniboxDropdownView::PaintChildren(const ui::PaintContext& context) {
  gfx::Rect contents_bounds = GetContentsBounds();
  // const int interior = GetLayoutConstant(OMNIBOX_DROPDOWN_BORDER_INTERIOR);
  const int interior = 0;
  contents_bounds.Inset(0, views::NonClientFrameView::kClientEdgeThickness, 0,
                        bottom_shadow_->height() - interior);

  ui::ClipRecorder clip_recorder(context);
  clip_recorder.ClipRect(contents_bounds);
  {
    ui::PaintRecorder recorder(context, size());
    SkColor background_color = Item_view_at(0)->GetColor(
        OmniboxDropdownEntryView::NORMAL, OmniboxDropdownEntryView::BACKGROUND);
    recorder.canvas()->DrawColor(background_color);
  }
  View::PaintChildren(context);
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownView, private:

views::View* OmniboxDropdownView::TargetForRect(views::View* root,
                                                     const gfx::Rect& rect) {
  CHECK_EQ(root, this);
  const gfx::Point point(rect.CenterPoint());
  size_t index = GetIndexForPoint(point);
  if (OmniboxDropdownModel::kNoMatch == index) {
    return this;
  }
  OmniboxDropdownEntryView* child = Item_view_at(index);
  gfx::Point point_in_child_coords(point);
  View::ConvertPointToTarget(this, child, &point_in_child_coords);
  return child->GetEventHandlerForPoint(point_in_child_coords);
}

size_t OmniboxDropdownView::GetIndexForPoint(
    const gfx::Point& point) {
  if (!HitTestPoint(point))
    return OmniboxDropdownModel::kNoMatch;

  int nb_match = EntryCountToShow();
  DCHECK(nb_match <= child_count());
  for (int i = 0; i < nb_match; ++i) {
    views::View* child = child_at(i);
    gfx::Point point_in_child_coords(point);
    View::ConvertPointToTarget(this, child, &point_in_child_coords);
    if (child->visible() && child->HitTestPoint(point_in_child_coords))
      return i;
  }
  return OmniboxDropdownModel::kNoMatch;
}

void OmniboxDropdownView::UpdateLineEvent(
    const ui::LocatedEvent& event,
    bool should_set_selected_line) {
  size_t index = GetIndexForPoint(event.location());
  model_->SetHoveredLine(index);
  model_->SetSelectedLine(index, false, false);
}

void OmniboxDropdownView::OpenSelectedLine(
    const ui::LocatedEvent& event,
    WindowOpenDisposition disposition) {
  size_t index = GetIndexForPoint(event.location());
  model_->OpenSelectedLine(index, disposition);
}

OmniboxDropdownEntryView* OmniboxDropdownView::Item_view_at(size_t i) {
  return static_cast<OmniboxDropdownEntryView*>(child_at(static_cast<int>(i)));
}

OmniboxDropdownEntryView* OmniboxDropdownView::GetItemByUrl(const GURL& url) {
  for (int i = 0; i < child_count(); ++i) {
    OmniboxDropdownEntryView* entry = Item_view_at(i);
    if (url == entry->GetUrl()) {
      return entry;
    }
  }
  return nullptr;
}

void OmniboxDropdownView::Observe(int type,
                    const content::NotificationSource& source,
                    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_MOUSE_PRESSED: {
      if (popup_ == NULL)  {
        return;
      }
      gfx::Point cursor_pos = display::Screen::GetScreen()->GetCursorScreenPoint();
      views::View::ConvertPointFromScreen(popup_->GetRootView(), &cursor_pos);
      bool in_location_bar_or_popup = cursor_pos.x() >= 0
             && cursor_pos.x() <= width()
             && cursor_pos.y() + location_bar_view_->height() >= 0
             && cursor_pos.y() <= height();

      if (!in_location_bar_or_popup)  {
        ClosePopup();
      }
      break;
    }
    default:
      NOTREACHED() << "Received unexpected notification " << type;
  }
}

void OmniboxDropdownView::Invalidate() {
  if (child_count()) {
    RemoveAllChildViews(true);
  }
  AddChildViews();
  target_bounds_.set_height(CalculatePopupHeight());
  popup_->SetBounds(GetPopupBounds());
  Layout();
}

size_t OmniboxDropdownView::EntryCountToShow() {
  return std::min(model_->Item_count(), kMaxEntryCountToShow);
}

void OmniboxDropdownView::UpdateEntry(const GURL& url) {
  OmniboxDropdownDataEntry* data = model_->GetEntryByUrl(url);
  OmniboxDropdownEntryView* entry = GetItemByUrl(url);
  if (data && entry) {
    entry->UpdateEntry(data);
  }
}