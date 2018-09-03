// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_LAST_TABS_LIST_UI_H_
#define CHROME_BROWSER_UI_WEBUI_LAST_TABS_LIST_UI_H_

#include "base/macros.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_types.h"
#include "content/public/browser/web_ui_controller.h"

class GURL;
class Profile;

namespace base {
class ListValue;
}

// The WebUI for nfsbrowser://last-tabs-list
class LastTabsListUI : public content::WebUIController {
 public:
  explicit LastTabsListUI(content::WebUI* web_ui);
  ~LastTabsListUI() override;

 private:
  // content::WebUIController implementation.
  bool OverrideHandleWebUIMessage(const GURL& source_url,
                                  const std::string& message,
                                  const base::ListValue& args) override;

  void OnGotSession(
      std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
      SessionID::id_type active_window_id);
  void GetRestoredURLs(
      std::vector<std::unique_ptr<sessions::SessionWindow>>* windows,
      base::ListValue* url_list);
  void RestoreTabsToBrowser(const sessions::SessionWindow& window,
                            base::ListValue* url_list);

  Profile* GetProfile() const;

  base::Closure quit_closure_for_sync_restore_;

  DISALLOW_COPY_AND_ASSIGN(LastTabsListUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_LAST_TABS_LIST_UI_H_
