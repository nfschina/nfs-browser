// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_MODEL_H_
#define COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_MODEL_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/omnibox/browser/omnibox_dropdown_data.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "third_party/skia/include/core/SkBitmap.h"

class OmniboxPopupView;
class Browser;

namespace gfx {
class Image;
}

class OmniboxDropdownModel {
 public:
  OmniboxDropdownModel(Browser* browser,
                      OmniboxPopupView* popup_view, OmniboxEditModel* edit_model);
  ~OmniboxDropdownModel();

  //Revert data.
  void Revert() ;

  OmniboxPopupView* view() const { return view_; }

  size_t hovered_line() const { return hovered_line_; }

  // Call to change the hovered line.  |line| should be within the range of
  // valid lines (to enable hover) or kNoMatch (to disable hover).
  void SetHoveredLine(size_t line);

  void SetButtonHoveredLine(size_t line);

  size_t selected_line() const { return selected_line_; }


  // Call to change the selected line.  This will update all state and repaint
  // the necessary parts of the window, as well as updating the edit with the
  // new temporary text.  |line| will be clamped to the range of valid lines.
  // |reset_to_default| is true when the selection is being reset back to the
  // default match, and thus there is no temporary text (and no
  // |manually_selected_match_|). If |force| is true then the selected line will
  // be updated forcibly even if the |line| is same as the current selected
  // line.
  // NOTE: This assumes the popup is open, and thus both old and new values for
  // the selected line should not be kNoMatch.
  void SetSelectedLine(size_t line, bool reset_to_default, bool force);


  // Immediately updates and opens the popup if necessary, then moves the
  // current selection down (|count| > 0) or up (|count| < 0), clamping to the
  // first or last result if necessary.  If |count| == 0, the selection will be
  // unchanged, but the popup will still redraw and modify the text in the
  // OmniboxEditModel.
  void Move(int count);

  void OpenSelectedLine(size_t line, WindowOpenDisposition disposition);

  size_t Item_count() { return popup_data_->Size(); }
  OmniboxDropdownDataEntry* DataEntry_at(size_t index) { return popup_data_->Entry_at(index); }
  OmniboxDropdownDataEntry* GetEmptyEntry() { return popup_data_->GetEmptyEntry(); }
  void RemoveEntry(size_t index) { popup_data_->RemoveEntry(index); }
  OmniboxDropdownDataEntry* GetEntryByUrl(const GURL& url) const {
    return popup_data_->GetEntryByUrl(url);
  }

  // Add and remove observers.
  void AddObserver(OmniboxDropdownDataObserver* observer);
  void RemoveObserver(OmniboxDropdownDataObserver* observer);

  void Load() { popup_data_->Load(); }

   // Computes the maximum width, in pixels, that can be allocated for the two
  // parts of an autocomplete result, i.e. the contents and the description.
  //
  // When |description_on_separate_line| is true, the caller will be displaying
  // two separate lines of text, so both contents and description can take up
  // the full available width. Otherwise, the contents and description are
  // assumed to be on the same line, with a separator between them.
  //
  // When |allow_shrinking_contents| is true, and the contents and description
  // are together on a line without enough space for both, the code tries to
  // divide the available space equally between the two, unless this would make
  // one or both too narrow. Otherwise, the contents is given as much space as
  // it wants and the description gets the remainder.
  static void ComputeMaxWidths(int contents_width,
                                    int separator_width,
                                    int description_width,
                                    int available_width,
                                    bool description_on_separate_line,
                                    bool allow_shrinking_contents,
                                    int* contents_max_width,
                                    int* description_max_width);

  // Returns true if the popup is currently open.
  bool IsOpen() const;

  void OnEscapeKeyPressed();

  // The token value for selected_line_, hover_line_ and functions dealing with
  // a "line number" that indicates "no line".
  static const size_t kNoMatch;

 private:
  OmniboxPopupView* view_;

  OmniboxEditModel* edit_model_;

  // The line that's currently hovered.  If we're not drawing a hover rect,
  // this will be kNoMatch, even if the cursor is over the popup contents.
  size_t hovered_line_;

  // The currently selected line.  This is kNoMatch when nothing is selected,
  // which should only be true when the popup is closed.
  size_t selected_line_;

  std::unique_ptr<OmniboxDropdownData> popup_data_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxDropdownModel);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_MODEL_H_
