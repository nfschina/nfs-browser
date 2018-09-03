// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/grit/generated_resources.h"
//#include "nfsbrowser/grit/nfsbrowser_thumbnails_resources.h"
#include "components/omnibox/browser/omnibox_dropdown_data.h"
#include "components/history/core/browser/top_sites.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/resources/grit/ui_resources_nfs.h"

namespace {
  const size_t kMostVisitedCount = 10;

  struct prepopulatePage {
    const char* url;
    const int favicon_id;
  } kPrepopulatePages[] = {
      {"https://www.baidu.com/", IDD_BOOKMARK_BAR_FAVICON_BAIDU},
      {"http://www.weibo.com/", IDD_BOOKMARK_BAR_FAVICON_XINLANG},
      {"http://www.taobao.com/", IDD_BOOKMARK_BAR_FAVICON_TAOBAO},
      {"http://www.sina.com.cn/", IDD_BOOKMARK_BAR_FAVICON_SINA},
      {"http://www.qzone.com/", IDD_BOOKMARK_BAR_FAVICON_QZONE},
      {"http://www.youku.com/", IDD_BOOKMARK_BAR_FAVICON_YOUKU},
      {"http://www.jd.com/", IDD_BOOKMARK_BAR_FAVICON_JINGDONG},
      {"http://www.iqiyi.com/", IDD_BOOKMARK_BAR_FAVICON_AIQIYI}
    };

} //namespace

OmniboxDropdownDataEntry::OmniboxDropdownDataEntry(base::string16 content,
              base::string16 description, gfx::ImageSkia icon, GURL url)
    :content(content),
    description(description),
    icon(icon),
    url(url),
    ready(false) {
}

 OmniboxDropdownDataEntry::~OmniboxDropdownDataEntry() {
 }

OmniboxDropdownData::OmniboxDropdownData(Browser* browser)
  : empty_entry_(new OmniboxDropdownDataEntry(l10n_util::GetStringUTF16(IDS_DROPDOWNVIEW_EMPTY_PROPMT),
                      base::ASCIIToUTF16(""), gfx::ImageSkia(), GURL())),
  browser_(browser),
  top_sites_(nullptr),
  scoped_observer_(this),
  weak_ptr_factory_(this) {
  Init();
}

OmniboxDropdownData::~OmniboxDropdownData() {
}

void OmniboxDropdownData::Init() {
  Profile* primary_profile = ProfileManager::GetPrimaryUserProfile();
  top_sites_ = TopSitesFactory::GetForProfile(primary_profile);
  if (top_sites_) {
    // Register as TopSitesObserver so that we can update ourselves when the
    // TopSites changes.
    scoped_observer_.Add(top_sites_.get());
  }

}

void OmniboxDropdownData::GetTopSitesData(DataChangedReason reason) {
  DCHECK(top_sites_);
  switch (reason) {
    case LOAD:
      top_sites_->GetMostVisitedURLs(
              base::Bind(&OmniboxDropdownData::OnTopSitesLoaded,
                         weak_ptr_factory_.GetWeakPtr()), false);
      break;

    case CHANGE:
      top_sites_->GetMostVisitedURLs(
              base::Bind(&OmniboxDropdownData::OnTopSitesReceived,
                         weak_ptr_factory_.GetWeakPtr()), false);
        break;

    default:
        break;
  }
}

void OmniboxDropdownData::Load() {
  GetTopSitesData(LOAD);
}

void OmniboxDropdownData::RemoveEntry(size_t index) {
  DCHECK(index >= 0);
  const GURL url = Entries_[index]->url;
  Entries_.erase(Entries_.begin() + index);
  top_sites_->AddBlacklistedURL(url);
}

void OmniboxDropdownData::TopSitesLoaded(history::TopSites* top_sites) {
}

void OmniboxDropdownData::TopSitesChanged(history::TopSites* top_sites,
                                       ChangeReason change_reason) {
  GetTopSitesData(CHANGE);
}

void OmniboxDropdownData::OnTopSitesLoaded(
        const history::MostVisitedURLList& visited_list) {
  set_data(visited_list);
  FOR_EACH_OBSERVER(OmniboxDropdownDataObserver, observers_,
                      OnOmniboxDropdownDataLoaded());
}

void OmniboxDropdownData::OnTopSitesReceived(
        const history::MostVisitedURLList& visited_list) {
  set_data(visited_list);
  FOR_EACH_OBSERVER(OmniboxDropdownDataObserver, observers_,
                    OnOmniboxDropdownDataChanged());
}

void  OmniboxDropdownData::set_data(
        const history::MostVisitedURLList& visited_list) {
  std::map<GURL, size_t> all_old_entries;
  for (size_t i = 0; i < Entries_.size(); i++) {
    all_old_entries[Entries_[i]->url] = i;
  }

  std::vector<GURL> update_entries;
  size_t size = visited_list.size();
  size_t valid_count = 0;
  std::vector<std::unique_ptr<OmniboxDropdownDataEntry> > new_entries;
  for (size_t i = 0; i < size && valid_count < kMostVisitedCount; ++i) {
    const history::MostVisitedURL& visited = visited_list[i];
    if (visited.url.spec().empty() || visited.title.empty()) {
      continue;
    }
    std::map<GURL, size_t>::iterator found = all_old_entries.find(visited_list[i].url);
    if (found != all_old_entries.end()) { //found
      if (Entries_[found->second]->description != visited_list[i].title &&
          !visited_list[i].title.empty()) {
        Entries_[found->second]->description = visited_list[i].title;
        update_entries.push_back(found->first);
      }
      new_entries.push_back(std::move(Entries_[found->second]));
    } else {
      std::string url_string = visited.url.possibly_invalid_spec();
      std::unique_ptr<OmniboxDropdownDataEntry> entry(
          new OmniboxDropdownDataEntry(base::UTF8ToUTF16(url_string),
              visited.title,
              gfx::ImageSkia(),
              visited.url));
      new_entries.push_back(std::move(entry));
    }
    valid_count++;
  }

  Entries_.clear();
  for (size_t i = 0; i < new_entries.size(); i++) {
    Entries_.push_back(std::move(new_entries[i]));
  }

  FOR_EACH_OBSERVER(OmniboxDropdownDataObserver, observers_,
      OnOmniboxDropdownEntryUpdate(update_entries));

  for (size_t i = 0; i < Entries_.size(); i++) {
    if (Entries_[i]->icon.isNull()) {
      Load_icon(Entries_[i]->url);
    }
  }
}

void OmniboxDropdownData::Load_icon(const GURL& url) {
   // Start request to fetch actual icon if possible.
  favicon::FaviconService* favicon_service =
      FaviconServiceFactory::GetForProfile(browser_->profile(),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service) {
    return;
  }

  favicon_service->GetFaviconImageForPageURL(
      url,
      base::Bind(&OmniboxDropdownData::OnFaviconDataAvailable,
                 weak_ptr_factory_.GetWeakPtr(), url), &cancelable_task_tracker_);
}

void OmniboxDropdownData::OnFaviconDataAvailable(const GURL& url,
                  const favicon_base::FaviconImageResult& image_result) {
  if (image_result.image.IsEmpty()) {
    size_t i = 0;
    for (; i < arraysize(kPrepopulatePages); i++) {
      //If it's prepopulate page
      if (!(url.spec().compare(kPrepopulatePages[i].url))) {
         OmniboxDropdownDataEntry* entry = GetEntryByUrl(url);
         if (entry) {
            entry->icon = *(ui::ResourceBundle::GetSharedInstance().
                             GetImageSkiaNamed(kPrepopulatePages[i].favicon_id));
         }
         break;
      }
    }
  } else {
    OmniboxDropdownDataEntry* entry = GetEntryByUrl(url);
    if (entry) {
      entry->icon = image_result.image.AsImageSkia();
    }
  }

  std::vector<GURL> urls;
  urls.push_back(url);
  FOR_EACH_OBSERVER(OmniboxDropdownDataObserver, observers_,
                    OnOmniboxDropdownEntryUpdate(urls));
}

size_t OmniboxDropdownData::Size() const {
  return Entries_.size();
 }

OmniboxDropdownDataEntry* OmniboxDropdownData::Entry_at(size_t index) {
  DCHECK(index >= 0);
  if (index >= Entries_.size()) {
    DCHECK(false);
    return NULL;
  }
  return Entries_[index].get();
}

OmniboxDropdownDataEntry* OmniboxDropdownData::GetEntryByUrl(
    const GURL& url) const {
  for (size_t i= 0; i < Size(); ++i) {
    if (Entries_[i]->url == url) {
      return Entries_[i].get();
    }
  }
  return nullptr;
}

void OmniboxDropdownData::AddObserver(OmniboxDropdownDataObserver* observer) {
  observers_.AddObserver(observer);
}

void OmniboxDropdownData::RemoveObserver(OmniboxDropdownDataObserver* observer) {
  observers_.RemoveObserver(observer);
}
