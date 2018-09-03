// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_DATA_H_
#define COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_DATA_H_

#include <stddef.h>
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/synchronization/lock.h"
#include "base/observer_list.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon/core/favicon_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/omnibox/browser/omnibox_dropdown_data_observer.h"
#include "url/gurl.h"
#include "ui/gfx/image/image_skia.h"

namespace history {
class TopSites;
}

namespace gfx {
  class ImageSkia;
}

class Browser;

enum DataChangedReason {
    LOAD,
    CHANGE
};

struct OmniboxDropdownDataEntry {
    base::string16 content;
    base::string16 description;
    gfx::ImageSkia icon;
    GURL url;
    bool ready;

    OmniboxDropdownDataEntry(base::string16 content,
              base::string16 description, gfx::ImageSkia icon, GURL url);
    ~OmniboxDropdownDataEntry();
};

class OmniboxDropdownData : public history::TopSitesObserver {
  public:
    OmniboxDropdownData(Browser* browser);
    ~OmniboxDropdownData() override;

    size_t Size() const;
    OmniboxDropdownDataEntry* Entry_at(size_t index);
    OmniboxDropdownDataEntry* GetEmptyEntry() { return empty_entry_.get(); }
    OmniboxDropdownDataEntry* GetEntryByUrl(const GURL& url) const;

    // Add and remove observers.
    void AddObserver(OmniboxDropdownDataObserver* observer);
    void RemoveObserver(OmniboxDropdownDataObserver* observer);

    void Load();

    void RemoveEntry(size_t index);

  private:
    void Init();

    // Overridden from history::TopSitesObserver:
    void TopSitesLoaded(history::TopSites* top_sites) override;
    void TopSitesChanged(history::TopSites* top_sites,
                         ChangeReason change_reason) override;

    void GetTopSitesData(DataChangedReason reason);

    // Callback to receive data requested from GetTopSitesData().
    void OnTopSitesLoaded(const history::MostVisitedURLList& visited_list);
    void OnTopSitesReceived(const history::MostVisitedURLList& visited_list);

    void set_data(const history::MostVisitedURLList& visited_list);

    void Load_icon(const GURL& url);

    void OnFaviconDataAvailable(const GURL& url,
                    const favicon_base::FaviconImageResult& image_result);

    std::vector<std::unique_ptr<OmniboxDropdownDataEntry> > Entries_;

    std::unique_ptr<OmniboxDropdownDataEntry> empty_entry_;

    // base::Lock data_lock_;

    Browser* browser_;

    scoped_refptr<history::TopSites> top_sites_;

    ScopedObserver<history::TopSites, history::TopSitesObserver> scoped_observer_;

    // Observers.
    base::ObserverList<OmniboxDropdownDataObserver> observers_;

    // Used for loading favicons.
    base::CancelableTaskTracker cancelable_task_tracker_;

     // For callbacks may be run after destruction.
    base::WeakPtrFactory<OmniboxDropdownData> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxDropdownData);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_DROPDOWN_DATA_H_
