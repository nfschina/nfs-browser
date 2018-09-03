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

#ifndef CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_DROPDOWN_ENTRY_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_DROPDOWN_ENTRY_VIEW_H_

#include <stddef.h>

#include <vector>

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/vector_icon_types.h"
//#include "ui/views/controls/image_view.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/view_targeter_delegate.h"
#include "ui/views/masked_targeter_delegate.h"
#include "url/gurl.h"

class LocationBarView;
class OmniboxDropdownView;
struct OmniboxDropdownDataEntry;

namespace gfx {
class Canvas;
class RenderText;
}

namespace views {
 class ImageButton;
}


class OmniboxDropdownEntryView : public views::View,
                   public views::ButtonListener,
                   public views::ViewTargeterDelegate {
 public:
  // Keep these ordered from least dominant (normal) to most dominant
  // (selected).
  enum ItemViewState {
    NORMAL = 0,
    HOVERED,
    SELECTED,
    NUM_STATES
  };

  enum ColorKind {
    BACKGROUND = 0,
    TEXT,
    DIMMED_TEXT,
    URL,
    NUM_KINDS
  };

  OmniboxDropdownEntryView(OmniboxDropdownView* model,
                    LocationBarView* location_bar_view,
                    const gfx::FontList& font_list,
                    bool is_empy_entry=false);
  ~OmniboxDropdownEntryView() override;

  SkColor GetColor(ItemViewState state, ColorKind kind) const;

  void Invalidate();

  // Invoked when this result view has been selected.
  void OnSelected();

  // views::View:
  gfx::Size GetPreferredSize() const override;
  void GetAccessibleState(ui::AXViewState* state) override;

  ItemViewState GetState() const;

  // Returns the height of the text portion of the result view. In the base
  // class, this is the height of one line of text.
  virtual int GetTextHeight() const;

  // Returns the display width required for the match contents.
  int GetMatchContentsWidth() const;

  void SetEntryData(OmniboxDropdownDataEntry* data_entry) ;

  // views::ButtonListener
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  void UpdateEntry(OmniboxDropdownDataEntry* data);

  const GURL& GetUrl() const { return url_; }

  protected:
  enum RenderTextType {
    CONTENTS = 0,
    SEPARATOR,
    DESCRIPTION,
    NUM_TYPES
  };

  // Paints the given |match| using the RenderText instances |contents| and
  // |description| at offset |x| in the bounds of this view.
  virtual void PaintMatch(gfx::RenderText* contents,
                          gfx::RenderText* description,
                          gfx::Canvas* canvas,
                          int x) const;

  // Draws given |render_text| on |canvas| at given location (|x|, |y|).
  // |contents| indicates if the |render_text| is for the match contents,
  // separator, or description.  Additional properties from |match| are used to
  // render Infinite suggestions correctly.  If |max_width| is a non-negative
  // number, the text will be elided to fit within |max_width|.  Returns the x
  // position to the right of the string.
  int DrawRenderText(gfx::RenderText* render_text,
                     RenderTextType render_text_type,
                     gfx::Canvas* canvas,
                     int x,
                     int y,
                     int max_width) const;

  // Creates a RenderText with given |text| and rendering defaults.
  std::unique_ptr<gfx::RenderText> CreateRenderText(
      const base::string16& text) const;

  const gfx::Rect& text_bounds() const { return text_bounds_; }

 private:
  // views::View:
  const char* GetClassName() const override;

  const gfx::ImageSkia& GetIcon();

  // Utility function for creating vector icons.
  gfx::ImageSkia GetVectorIcon(gfx::VectorIconId icon_id) const;

   // Resets all RenderTexts for contents and description of the |match_| and its
  // associated keyword match.
  void ResetRenderTexts() const;


  // views::View:
  void Layout() override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void OnPaint(gfx::Canvas* canvas) override;

   int GetContentLineHeight() const;

   // Returns the necessary margin, if any, at the start and end of the view.
  // This allows us to keep the icon and text in the view aligned with the
  // location bar contents. For a left-to-right language, StartMargin()
  // and EndMargin() correspond to the left and right margins, respectively.
  int StartMargin() const;
  int EndMargin() const;

  std::unique_ptr<gfx::RenderText> CreateClassifiedRenderText(
      const base::string16& text) const;

  views::View* TargetForRect(View* root, const gfx::Rect& rect) override;

  static int default_icon_size_;

  // This row's model and model index.
  OmniboxDropdownView* model_;

  LocationBarView* location_bar_view_;

  const gfx::FontList font_list_;
  int font_height_;

  bool is_empy_entry_;

  // A context used for mirroring regions.
  class MirroringContext;
  std::unique_ptr<MirroringContext> mirroring_context_;

  class RemoveButton;

  gfx::Rect text_bounds_;
  gfx::Rect icon_bounds_;

  gfx::ImageSkia icon_;

  views::ImageButton* remove_button_;

  GURL url_;

  // We preserve these RenderTexts so that we won't recreate them on every call
  // to GetMatchContentsWidth() or OnPaint().
  mutable std::unique_ptr<gfx::RenderText> contents_rendertext_;
  mutable std::unique_ptr<gfx::RenderText> description_rendertext_;
  mutable std::unique_ptr<gfx::RenderText> separator_rendertext_;
  mutable int separator_width_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxDropdownEntryView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_DROPDOWN_ENTRY_VIEW_H_
