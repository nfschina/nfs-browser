// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/omnibox_dropdown_model.h"
#include "chrome/browser/ui/views/omnibox/omnibox_dropdown_view.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
//#include "components/bookmarks/browser/bookmark_model.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "third_party/icu/source/common/unicode/ubidi.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"

using bookmarks::BookmarkModel;

///////////////////////////////////////////////////////////////////////////////
// OmniboxDropdownModel

const size_t OmniboxDropdownModel::kNoMatch = static_cast<size_t>(-1);

OmniboxDropdownModel::OmniboxDropdownModel(Browser* browser,
    OmniboxPopupView* popup_view,
    OmniboxEditModel* edit_model)
    : view_(popup_view),
      edit_model_(edit_model),
      hovered_line_(kNoMatch),
      selected_line_(kNoMatch) {
    popup_data_.reset(new OmniboxDropdownData(browser));
    edit_model->set_dropdown_model(this);
}

OmniboxDropdownModel::~OmniboxDropdownModel() {
  popup_data_.reset();
}

void OmniboxDropdownModel::Revert() {
  hovered_line_ = kNoMatch;
  selected_line_ = kNoMatch;
}

void OmniboxDropdownModel::SetHoveredLine(size_t line) {
  const bool is_disabling = (line == kNoMatch);
  DCHECK(is_disabling || (line < Item_count()));

  if (line == hovered_line_)
    return;  // Nothing to do

  size_t prev_hovered_line = hovered_line_;
  hovered_line_ = line;

  // Make sure the old hovered line is redrawn.  No need to redraw the selected
  // line since selection overrides hover so the appearance won't change.
  if ((prev_hovered_line != kNoMatch) && (prev_hovered_line != selected_line_))
    view_->InvalidateLine(prev_hovered_line);

  // Change the hover to the new line.
  if (!is_disabling && (hovered_line_ != selected_line_))
    view_->InvalidateLine(hovered_line_);
}

void OmniboxDropdownModel::SetSelectedLine(size_t line,
                                        bool reset_to_default,
                                        bool force) {
  line = std::min(line, Item_count() - 1);
  bool is_empty = popup_data_->Size() == 0;
  if (is_empty)  {
    return;
  }
  OmniboxDropdownDataEntry* entry = is_empty ? popup_data_->GetEmptyEntry()
      : popup_data_->Entry_at(line);
  GURL current_destination(entry->url);
  const size_t prev_selected_line = selected_line_;
  selected_line_ = line;
  if (prev_selected_line != kNoMatch) {
    view_->InvalidateLine(prev_selected_line);
  }
  view_->InvalidateLine(selected_line_);
  view_->OnLineSelected(selected_line_);

  std::string url_string = entry->url.possibly_invalid_spec();
  edit_model_->OnPopupDataChanged(base::UTF8ToUTF16(url_string), &current_destination,
                                    base::string16(), false);

  // Repaint old and new selected lines immediately, so that the edit doesn't
  // appear to update [much] faster than the popup.
  view_->PaintUpdatesNow();
}


void OmniboxDropdownModel::Move(int count) {
  if (!Item_count())
    return;

  // The user is using the keyboard to change the selection, so stop tracking
  // hover.
  SetHoveredLine(kNoMatch);

  // Clamp the new line to [0, result_.count() - 1].
  const size_t new_line = selected_line_ + count;
  /*
  SetSelectedLine(((count < 0) && (new_line >= selected_line_)) ? 0 : new_line,
                  false, false);
  */
  SetSelectedLine((((count < 0) && (new_line >= selected_line_)) ||
           ((count < 0) && (selected_line_ == kNoMatch))) ?
                  (Item_count() + (count % Item_count())) : (new_line % Item_count()), false, false);
}

void OmniboxDropdownModel::OpenSelectedLine(size_t line, WindowOpenDisposition disposition) {
  edit_model_->AcceptInput(disposition, false);
}

void OmniboxDropdownModel::AddObserver(OmniboxDropdownDataObserver* observer) {
  popup_data_->AddObserver(observer);
}

void OmniboxDropdownModel::RemoveObserver(OmniboxDropdownDataObserver* observer) {
  popup_data_->RemoveObserver(observer);
}

// static
void OmniboxDropdownModel::ComputeMaxWidths(int contents_width,
                                              int separator_width,
                                              int description_width,
                                              int available_width,
                                              bool description_on_separate_line,
                                              bool allow_shrinking_contents,
                                              int* contents_max_width,
                                              int* description_max_width) {
  available_width = std::max(available_width, 0);
  *contents_max_width = std::min(contents_width, available_width);
  *description_max_width = std::min(description_width, available_width);

  // If the description is empty, or the contents and description are on
  // separate lines, each can get the full available width.
  if (!description_width || description_on_separate_line)
    return;

  // If we want to display the description, we need to reserve enough space for
  // the separator.
  available_width -= separator_width;
  if (available_width < 0) {
    *description_max_width = 0;
    return;
  }

  if (contents_width + description_width > available_width) {
    if (allow_shrinking_contents) {
      // Try to split the available space fairly between contents and
      // description (if one wants less than half, give it all it wants and
      // give the other the remaining space; otherwise, give each half).
      // However, if this makes the contents too narrow to show a significant
      // amount of information, give the contents more space.
      *contents_max_width = std::max(
          (available_width + 1) / 2, available_width - description_width);

      const int kMinimumContentsWidth = 300;
      *contents_max_width = std::min(
          std::min(std::max(*contents_max_width, kMinimumContentsWidth),
                   contents_width),
          available_width);
    }

    // Give the description the remaining space, unless this makes it too small
    // to display anything meaningful, in which case just hide the description
    // and let the contents take up the whole width.
    *description_max_width =
        std::min(description_width, available_width - *contents_max_width);
    const int kMinimumDescriptionWidth = 75;
    if (*description_max_width <
        std::min(description_width, kMinimumDescriptionWidth)) {
      *description_max_width = 0;
      // Since we're not going to display the description, the contents can have
      // the space we reserved for the separator.
      available_width += separator_width;
      *contents_max_width = std::min(contents_width, available_width);
    }
  }
}

bool OmniboxDropdownModel::IsOpen() const {
  return view_->IsOpen();
}

void OmniboxDropdownModel::OnEscapeKeyPressed() {
  view_->ClosePopup();
  view_->OnDragCanceled();
}

