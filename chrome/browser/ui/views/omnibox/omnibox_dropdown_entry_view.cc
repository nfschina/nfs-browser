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

// For WinDDK ATL compatibility, these ATL headers must come first.
#include "build/build_config.h"

#if defined(OS_WIN)
#include <atlbase.h>  // NOLINT
#include <atlwin.h>  // NOLINT
#endif

#include "base/macros.h"
#include "chrome/browser/ui/views/omnibox/omnibox_dropdown_entry_view.h"

#include <limits.h>

#include <algorithm>  // NOLINT

#include "base/i18n/bidi_line_iterator.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_dropdown_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_dropdown_model.h"
#include "grit/components_scaled_resources.h"
#include "grit/theme_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
//#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/render_text.h"
#include "ui/gfx/text_utils.h"
#include "ui/gfx/vector_icons_public.h"
#include "ui/native_theme/native_theme.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/resources/grit/ui_resources_nfs.h"
#include "ui/views/controls/button/image_button.h"

using ui::NativeTheme;

namespace {

// A mapping from OmniboxDropdownEntryView's ItemViewState/ColorKind types to
// NativeTheme colors.
struct TranslationTable {
  ui::NativeTheme::ColorId id;
  OmniboxDropdownEntryView::ItemViewState state;
  OmniboxDropdownEntryView::ColorKind kind;
} static const kTranslationTable[] = {
  { NativeTheme::kColorId_ResultsTableNormalBackground,
    OmniboxDropdownEntryView::NORMAL, OmniboxDropdownEntryView::BACKGROUND },
  { NativeTheme::kColorId_ResultsTableHoveredBackground,
    OmniboxDropdownEntryView::HOVERED, OmniboxDropdownEntryView::BACKGROUND },
  { NativeTheme::kColorId_ResultsTableSelectedBackground,
    OmniboxDropdownEntryView::SELECTED, OmniboxDropdownEntryView::BACKGROUND },
  { NativeTheme::kColorId_ResultsTableNormalText,
    OmniboxDropdownEntryView::NORMAL, OmniboxDropdownEntryView::TEXT },
  { NativeTheme::kColorId_ResultsTableHoveredText,
    OmniboxDropdownEntryView::HOVERED, OmniboxDropdownEntryView::TEXT },
  { NativeTheme::kColorId_ResultsTableSelectedText,
    OmniboxDropdownEntryView::SELECTED, OmniboxDropdownEntryView::TEXT },
  { NativeTheme::kColorId_ResultsTableNormalDimmedText,
    OmniboxDropdownEntryView::NORMAL, OmniboxDropdownEntryView::DIMMED_TEXT },
  { NativeTheme::kColorId_ResultsTableHoveredDimmedText,
    OmniboxDropdownEntryView::HOVERED, OmniboxDropdownEntryView::DIMMED_TEXT },
  { NativeTheme::kColorId_ResultsTableSelectedDimmedText,
    OmniboxDropdownEntryView::SELECTED, OmniboxDropdownEntryView::DIMMED_TEXT },
  { NativeTheme::kColorId_ResultsTableNormalUrl,
    OmniboxDropdownEntryView::NORMAL, OmniboxDropdownEntryView::URL },
  { NativeTheme::kColorId_ResultsTableHoveredUrl,
    OmniboxDropdownEntryView::HOVERED, OmniboxDropdownEntryView::URL },
  { NativeTheme::kColorId_ResultsTableSelectedUrl,
    OmniboxDropdownEntryView::SELECTED, OmniboxDropdownEntryView::URL },
};

const SkColor kContents_rendertext_color = SkColorSetRGB(0x4f, 0xa7, 0xff);

const SkColor kDescription_rendertext_color = SkColorSetRGB(0x99, 0x99, 0x99);

const int kDescriptionTextMargingLeft = 50;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownEntryView, public:

// This class is a utility class for calculations affected by whether the result
// view is horizontally mirrored.  The drawing functions can be written as if
// all drawing occurs left-to-right, and then use this class to get the actual
// coordinates to begin drawing onscreen.
class OmniboxDropdownEntryView::MirroringContext {
 public:
  MirroringContext() : center_(0), right_(0) {}

  // Tells the mirroring context to use the provided range as the physical
  // bounds of the drawing region.  When coordinate mirroring is needed, the
  // mirror point will be the center of this range.
  void Initialize(int x, int width) {
    center_ = x + width / 2;
    right_ = x + width;
  }

  // Given a logical range within the drawing region, returns the coordinate of
  // the possibly-mirrored "left" side.  (This functions exactly like
  // View::MirroredLeftPointForRect().)
  int mirrored_left_coord(int left, int right) const {
    return base::i18n::IsRTL() ? (center_ + (center_ - right)) : left;
  }

  // Given a logical coordinate within the drawing region, returns the remaining
  // width available.
  int remaining_width(int x) const {
    return right_ - x;
  }

 private:
  int center_;
  int right_;

  DISALLOW_COPY_AND_ASSIGN(MirroringContext);
};

class OmniboxDropdownEntryView::RemoveButton: public views::ImageButton {
 public:
  RemoveButton(views::ButtonListener* listener)
    :ImageButton(listener) {}

  ~RemoveButton() override {}

  void VisibilityChanged(View* starting_from, bool visible) override {
    Button::VisibilityChanged(starting_from, visible);
    if (state() == Button::STATE_DISABLED)
      return;
    SetState(visible? STATE_HOVERED:STATE_NORMAL);
}

  const char* GetClassName() const override { return "RemoveButton"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoveButton);
};

OmniboxDropdownEntryView::OmniboxDropdownEntryView(
                                      OmniboxDropdownView* model,
                                     LocationBarView* location_bar_view,
                                     const gfx::FontList& font_list,
                                     bool is_empy_entry)
    : model_(model),
      location_bar_view_(location_bar_view),
      font_list_(font_list),
      font_height_(
          std::max(font_list.GetHeight(),
                   font_list.DeriveWithWeight(gfx::Font::Weight::BOLD).GetHeight())),
      is_empy_entry_(is_empy_entry),
      mirroring_context_(new MirroringContext()),
      remove_button_(new RemoveButton(this)) {
  if (default_icon_size_ == 0) {
    default_icon_size_ =
        location_bar_view_->GetThemeProvider()->GetImageSkiaNamed(
            AutocompleteMatch::TypeToIcon(
                AutocompleteMatchType::URL_WHAT_YOU_TYPED))->width();
  }

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  remove_button_->SetImage(views::Button::STATE_NORMAL,
                    rb.GetImageNamed(IDD_TAB_CLOSE_N_DARK).ToImageSkia());
  remove_button_->SetImage(views::Button::STATE_HOVERED,
                    rb.GetImageNamed(IDD_TAB_CLOSE_P_DARK).ToImageSkia());
  remove_button_->SetImage(views::Button::STATE_PRESSED,
                    rb.GetImageNamed(IDD_TAB_CLOSE_P_DARK).ToImageSkia());

  remove_button_->SetVisible(false);
  AddChildView(remove_button_);

  SetEventTargeter(
        std::unique_ptr<views::ViewTargeter>(new views::ViewTargeter(this)));

  // So we get don't get enter/exit on children and don't prematurely stop the
  // hover.
  set_notify_enter_exit_on_child(true);
}

OmniboxDropdownEntryView::~OmniboxDropdownEntryView() {
}

SkColor OmniboxDropdownEntryView::GetColor(
    ItemViewState state,
    ColorKind kind) const {
  for (size_t i = 0; i < arraysize(kTranslationTable); ++i) {
    if (kTranslationTable[i].state == state &&
        kTranslationTable[i].kind == kind) {
      return GetNativeTheme()->GetSystemColor(kTranslationTable[i].id);
    }
  }

  NOTREACHED();
  return gfx::kPlaceholderColor;
}

void OmniboxDropdownEntryView::Invalidate() {
  // While the text in the RenderTexts may not have changed, the styling
  // (color/bold) may need to change. So we reset them to cause them to be
  // recomputed in OnPaint().
  //ResetRenderTexts();
  const ItemViewState state = GetState();
  if (state != NORMAL) {
    remove_button_->SetVisible(true);
  } else {
    remove_button_->SetVisible(false);
  }
  Layout();
  SchedulePaint();
}

void OmniboxDropdownEntryView::OnSelected() {
  DCHECK_EQ(SELECTED, GetState());
}

gfx::Size OmniboxDropdownEntryView::GetPreferredSize() const {
  return gfx::Size(0, GetContentLineHeight());
}

void OmniboxDropdownEntryView::GetAccessibleState(ui::AXViewState* state) {

}

void OmniboxDropdownEntryView::ButtonPressed(views::Button* sender,
    const ui::Event& event) {
  size_t index = model_->GetIndexByUrl(url_);
  if (index != OmniboxDropdownModel::kNoMatch) {
    model_->RemoveEntry(index);
  }
}

void OmniboxDropdownEntryView::UpdateEntry(
    OmniboxDropdownDataEntry* data) {
  ResetRenderTexts();
  SetEntryData(data);
  Invalidate();
}

////////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownEntryView, protected:

OmniboxDropdownEntryView::ItemViewState OmniboxDropdownEntryView::GetState() const {
  size_t index = model_->GetIndexByUrl(url_);
  if (index != OmniboxDropdownModel::kNoMatch) {
    if (model_->IsSelectedIndex(index)) {
      return SELECTED;
    }
    return model_->IsHoveredIndex(index) ? HOVERED : NORMAL;
  }
  return NORMAL;
}

int OmniboxDropdownEntryView::GetTextHeight() const {
  return font_height_;
}

void OmniboxDropdownEntryView::PaintMatch(gfx::RenderText* contents,
                                   gfx::RenderText* description,
                                   gfx::Canvas* canvas,
                                   int x) const {
  int y = text_bounds_.y();

  if (!separator_rendertext_) {
    const base::string16& separator =
        l10n_util::GetStringUTF16(IDS_AUTOCOMPLETE_MATCH_DESCRIPTION_SEPARATOR);
    separator_rendertext_.reset(CreateRenderText(separator).release());
    separator_rendertext_->SetColor(GetColor(GetState(), DIMMED_TEXT));
    separator_width_ = separator_rendertext_->GetContentWidth();
  }

  contents->SetColor(kContents_rendertext_color);
  contents->SetDisplayRect(gfx::Rect(gfx::Size(INT_MAX, 0)));
  if (description) {
    description->SetColor(kDescription_rendertext_color);
    description->SetDisplayRect(gfx::Rect(gfx::Size(INT_MAX, 0)));
  }
  int contents_max_width, description_max_width;
  // OmniboxDropdownModel::ComputeMaxWidths(
  //     contents->GetContentWidth(),
  //     separator_width_,
  //     description ? description->GetContentWidth() : 0,
  //     mirroring_context_->remaining_width(x),
  //     false,
  //     false,
  //     &contents_max_width,
  //     &description_max_width);
  contents_max_width = remove_button_->visible() ?
      (text_bounds_.width() + remove_button_->GetPreferredSize().width()) / 2
      : text_bounds_.width() / 2;
  description_max_width =  description ?
      text_bounds_.width() - contents_max_width : 0;

  int after_contents_x = DrawRenderText(contents, CONTENTS, canvas,
                                        x, y, contents_max_width);

  if (description_max_width != 0) {
    // x = DrawRenderText(separator_rendertext_.get(), SEPARATOR, canvas,
    //                      after_contents_x, y, separator_width_);
    x = after_contents_x + kDescriptionTextMargingLeft;
    description_max_width = std::max(
        description_max_width - kDescriptionTextMargingLeft, 0);
    DrawRenderText(description, DESCRIPTION, canvas, x, y,
                   description_max_width);
  }
}

int OmniboxDropdownEntryView::DrawRenderText(
    gfx::RenderText* render_text,
    RenderTextType render_text_type,
    gfx::Canvas* canvas,
    int x,
    int y,
    int max_width) const {
  int right_x = x + max_width;

  // Set the display rect to trigger eliding.
  const int height = GetContentLineHeight();
  render_text->SetDisplayRect(
      gfx::Rect(mirroring_context_->mirrored_left_coord(x, right_x), y,
                right_x - x, height));
  render_text->Draw(canvas);
  return right_x;
}

std::unique_ptr<gfx::RenderText> OmniboxDropdownEntryView::CreateRenderText(
    const base::string16& text) const {
  std::unique_ptr<gfx::RenderText> render_text(
      gfx::RenderText::CreateInstance());
  render_text->SetDisplayRect(gfx::Rect(gfx::Size(INT_MAX, 0)));
  render_text->SetCursorEnabled(false);
  render_text->SetElideBehavior(gfx::ELIDE_TAIL);
  render_text->SetFontList(font_list_);
  render_text->SetText(text);
  return render_text;
}

void OmniboxDropdownEntryView::SetEntryData(OmniboxDropdownDataEntry* data_entry) {
  if (!contents_rendertext_) {
    contents_rendertext_.reset(
        CreateClassifiedRenderText(
            data_entry->content).release());
  }

  if (!description_rendertext_) {
    description_rendertext_.reset(
        CreateClassifiedRenderText(
            data_entry->description).release());
  }

  icon_  = data_entry->icon;
  url_ = data_entry->url;
}

std::unique_ptr<gfx::RenderText> OmniboxDropdownEntryView::CreateClassifiedRenderText(
    const base::string16& text) const {
  std::unique_ptr<gfx::RenderText> render_text(CreateRenderText(text));

  return render_text;
}

// static
int OmniboxDropdownEntryView::default_icon_size_ = 0;

const char* OmniboxDropdownEntryView::GetClassName() const {
  return "OmniboxDropdownEntryView";
}

const gfx::ImageSkia& OmniboxDropdownEntryView::GetIcon() {
  if (icon_.isNull()) {
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    icon_ = *(rb.GetImageSkiaNamed(IDR_DEFAULT_FAVICON));
  }
  return icon_;
}

gfx::ImageSkia OmniboxDropdownEntryView::GetVectorIcon(
    gfx::VectorIconId icon_id) const {
  return gfx::CreateVectorIcon(icon_id, 16, color_utils::DeriveDefaultIconColor(
                                                GetColor(GetState(), TEXT)));
}

void OmniboxDropdownEntryView::ResetRenderTexts() const {
  contents_rendertext_.reset();
  description_rendertext_.reset();
  separator_rendertext_.reset();
}

void OmniboxDropdownEntryView::Layout() {
  const int horizontal_padding =
      GetLayoutConstant(LOCATION_BAR_HORIZONTAL_PADDING);
  const int start_x = StartMargin() + horizontal_padding;
  const int end_x = width() - EndMargin() - horizontal_padding;

  const gfx::ImageSkia icon = GetIcon();
  // Pre-MD, normal icons are 19 px wide, while extension icons are 16 px wide.
  // The code in IconLabelBubbleView::Layout() positions these icons in the
  // omnibox using ICON_LABEL_VIEW_TRAILING_PADDING, so we use that here as well
  // so the icons will line up.
  //
  // Technically we don't need the IsModeMaterial() check here, but it will make
  // it easier to see that all this code is dead once we switch to MD.
  int icon_x = start_x;
  if (!ui::MaterialDesignController::IsModeMaterial() &&
      (icon.width() != default_icon_size_))
    icon_x += IconLabelBubbleView::kTrailingPaddingPreMd;
  icon_bounds_.SetRect(icon_x, (GetContentLineHeight() - icon.height()) / 2,
                       icon.width(), icon.height());

  bool button_visible = remove_button_->visible();
  int text_x = start_x + default_icon_size_ + horizontal_padding;
  text_x = is_empy_entry_ ? start_x : text_x;
  int text_width = end_x - text_x - horizontal_padding;
  if (button_visible) {
    text_width -= remove_button_->GetPreferredSize().width();
  }
  text_bounds_.SetRect(text_x, 0, std::max(text_width, 0), height());

  if (button_visible) {
    int button_x = text_x + text_width + horizontal_padding;
    int button_y = std::max(
        (height() - remove_button_->GetPreferredSize().height()) / 2, 0);
    remove_button_->SetBounds(button_x, button_y,
        remove_button_->GetPreferredSize().width(),
        remove_button_->GetPreferredSize().height());
  }
}

void OmniboxDropdownEntryView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
}

void OmniboxDropdownEntryView::OnPaint(gfx::Canvas* canvas) {
  const ItemViewState state = GetState();
  if (state != NORMAL)
    canvas->DrawColor(GetColor(state, BACKGROUND));

  // NOTE: While animating the keyword match, both matches may be visible.
  if (!is_empy_entry_) {
    canvas->DrawImageInt(GetIcon(), GetMirroredXForRect(icon_bounds_),
                       icon_bounds_.y());
  }
  int x = GetMirroredXForRect(text_bounds_);
  mirroring_context_->Initialize(x, text_bounds_.width());
  if (contents_rendertext_.get() && description_rendertext_.get()) {
    PaintMatch(contents_rendertext_.get(),
             description_rendertext_.get(), canvas, x);
  }
}

int OmniboxDropdownEntryView::GetContentLineHeight() const {
  if (is_empy_entry_) {
    return GetTextHeight();
  } else {
    return std::max(
      default_icon_size_ + GetLayoutInsets(OMNIBOX_DROPDOWN_ICON).height(),
      GetTextHeight() + GetLayoutInsets(OMNIBOX_DROPDOWN_TEXT).height());
  }
}

int OmniboxDropdownEntryView::StartMargin() const {
  return 0;
}

int OmniboxDropdownEntryView::EndMargin() const {
  return 0;
}

views::View* OmniboxDropdownEntryView::TargetForRect(
    View* root, const gfx::Rect& rect) {
  const gfx::Point point(rect.CenterPoint());
  views::View* child = child_at(0);
  gfx::Point point_in_child_coords(point);
  View::ConvertPointToTarget(this, child, &point_in_child_coords);
  if (child->visible() && child->HitTestPoint(point_in_child_coords))
    return child;
  return parent();
}