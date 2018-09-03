// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_DATA_OBSERVER_H_
#define COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_DATA_OBSERVER_H_

class OmniboxDropdownDataObserver {
 public:
  virtual void OnOmniboxDropdownDataLoaded() = 0;
  virtual void OnOmniboxDropdownDataChanged() = 0;
  virtual void OnOmniboxDropdownEntryUpdate(
      const std::vector<GURL>& urls) = 0;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_DATA_OBSERVER_H_
