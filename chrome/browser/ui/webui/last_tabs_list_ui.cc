// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/last_tabs_list_ui.h"

#include "base/run_loop.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/browser_resources.h"
#include "grit/components_strings.h"
#include "grit/generated_resources.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace {

int GetNavigationIndexToSelect(const sessions::SessionTab& tab) {
  DCHECK(!tab.navigations.empty());
  const int selected_index =
      std::max(0, std::min(tab.current_navigation_index,
                           static_cast<int>(tab.navigations.size() - 1)));

  // After user sign out, Chrome may navigate to the setting page from the
  // sign out page asynchronously. The browser may be closed before the
  // navigation callback finished.
  std::string setting_page_url = std::string(chrome::kChromeUISettingsURL);
  std::string sign_out_page_url =
      setting_page_url + std::string(chrome::kSignOutSubPage);
  if (selected_index > 0 &&
      tab.navigations[selected_index].virtual_url().spec() ==
          sign_out_page_url &&
      tab.navigations[selected_index - 1].virtual_url().spec() ==
          setting_page_url) {
    return selected_index - 1;
  }

  return selected_index;
}

void OpenURLInCurrentTab(Profile* profile, const GURL& url) {
  chrome::NavigateParams params(profile, url,
                                ui::PAGE_TRANSITION_AUTO_BOOKMARK);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
  DCHECK(chrome::FindLastActive());
  params.source_contents = chrome::FindLastActive()->
      tab_strip_model()->GetActiveWebContents();
  chrome::Navigate(&params);
}

}

LastTabsListUI::LastTabsListUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  content::WebUIDataSource* html_source =
    content::WebUIDataSource::Create(chrome::kChromeUILastTabsListHost);

  // Add required resources.
  html_source->AddLocalizedString("title", IDS_LAST_TABS_LIST_UI_TITLE);

  html_source->SetJsonPath("strings.js");

  html_source->AddResourcePath("last_tabs_list.css", IDR_LAST_TABS_LIST_CSS);
  html_source->AddResourcePath("last_tabs_list.js", IDR_LAST_TABS_LIST_JS);
  html_source->SetDefaultResource(IDR_LAST_TABS_LIST_HTML);

  Profile* profile = GetProfile();
  content::WebUIDataSource::Add(profile, html_source);
}

LastTabsListUI::~LastTabsListUI() {
}

bool LastTabsListUI::OverrideHandleWebUIMessage(const GURL& source_url,
                                                const std::string& message,
                                                const base::ListValue& args) {
  if ("restore" != message) {
    return false;
  }

  SessionService* session_service =
      SessionServiceFactory::GetForProfile(GetProfile());
  DCHECK(session_service);
  if (!session_service) {
    OpenURLInCurrentTab(GetProfile(), GURL(chrome::kChromeUINewTabURL));
    return true;
  }

  base::CancelableTaskTracker cancelable_task_tracker;
  session_service->GetLastSession(
      base::Bind(&LastTabsListUI::OnGotSession, base::Unretained(this)),
      &cancelable_task_tracker);

  base::MessageLoop::ScopedNestableTaskAllower allow(
      base::MessageLoop::current());
  base::RunLoop loop;
  quit_closure_for_sync_restore_ = loop.QuitClosure();
  loop.Run();
  quit_closure_for_sync_restore_ = base::Closure();
  return true;
}

void LastTabsListUI::OnGotSession(
    std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
    SessionID::id_type active_window_id) {
  CHECK(!quit_closure_for_sync_restore_.is_null());
  if (quit_closure_for_sync_restore_.is_null()) {
    OpenURLInCurrentTab(GetProfile(), GURL(chrome::kChromeUINewTabURL));
  }
  quit_closure_for_sync_restore_.Run();

  if (windows.empty()) {
    OpenURLInCurrentTab(GetProfile(), GURL(chrome::kChromeUINewTabURL));
  }

  base::ListValue url_list;
  GetRestoredURLs(&windows, &url_list);
  if (url_list.empty()) {
    OpenURLInCurrentTab(GetProfile(), GURL(chrome::kChromeUINewTabURL));
  }
  // TODO: 是否需要考虑url_list长度为1的情况

  web_ui()->CallJavascriptFunctionUnsafe("ltl.onRestore", url_list);
}

void LastTabsListUI::GetRestoredURLs(
    std::vector<std::unique_ptr<sessions::SessionWindow>>* windows,
    base::ListValue* url_list) {
  for (auto i = windows->begin(); i != windows->end(); ++i) {
    RestoreTabsToBrowser(*(*i), url_list);
  }
}

void LastTabsListUI::RestoreTabsToBrowser(
    const sessions::SessionWindow& window,
    base::ListValue* url_list) {
  for (int i = 0; i < static_cast<int>(window.tabs.size()); ++i) {
    const sessions::SessionTab& tab = *(window.tabs[i]);

    GURL restore_url = tab.navigations.at(
        GetNavigationIndexToSelect(tab)).virtual_url();
    base::string16 title = tab.navigations.at(
        GetNavigationIndexToSelect(tab)).title();

    // TODO: 是否需要考虑网址为"nfsbrowser://last-tabs-list"的情况
    std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue);
    value->SetString("url", restore_url.spec());
    if (0 < title.size()) {
      value->SetString("title", title);
    } else {
      value->SetString("title", restore_url.spec());
    }

    url_list->Append(std::move(value));
  }
}

Profile* LastTabsListUI::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}
