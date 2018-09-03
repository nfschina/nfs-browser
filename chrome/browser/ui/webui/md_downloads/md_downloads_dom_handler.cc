// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/md_downloads/md_downloads_dom_handler.h"

#include <algorithm>
#include <functional>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/supports_user_data.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/download_danger_prompt.h"
#include "chrome/browser/download/download_history.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/download/download_query.h"
#include "chrome/browser/download/download_service.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/download/drag_download_item.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/webui/fileicon_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/browser/byte_stream.h"
#include "content/browser/download/download_create_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_danger_type.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "grit/generated_resources.h"
#include "net/base/filename_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/gfx/image/image.h"
#include "chrome/browser/ui/singleton_tabs.h"

using base::UserMetricsAction;
using content::BrowserThread;

const int kUpdatePeriodMs = 1000;

namespace {

enum DownloadsDOMEvent {
  DOWNLOADS_DOM_EVENT_GET_DOWNLOADS = 0,
  DOWNLOADS_DOM_EVENT_OPEN_FILE = 1,
  DOWNLOADS_DOM_EVENT_DRAG = 2,
  DOWNLOADS_DOM_EVENT_SAVE_DANGEROUS = 3,
  DOWNLOADS_DOM_EVENT_DISCARD_DANGEROUS = 4,
  DOWNLOADS_DOM_EVENT_SHOW = 5,
  DOWNLOADS_DOM_EVENT_PAUSE = 6,
  DOWNLOADS_DOM_EVENT_REMOVE = 7,
  DOWNLOADS_DOM_EVENT_CANCEL = 8,
  DOWNLOADS_DOM_EVENT_CLEAR_ALL = 9,
  DOWNLOADS_DOM_EVENT_OPEN_FOLDER = 10,
  DOWNLOADS_DOM_EVENT_RESUME = 11,
  DOWNLOADS_DOM_EVENT_NEWITEM = 12,
  DOWNLOADS_DOM_EVENT_OPENSETTING = 13,
  DOWNLOADS_DOM_EVENT_MAX
};

void CountDownloadsDOMEvents(DownloadsDOMEvent event) {
  UMA_HISTOGRAM_ENUMERATION("Download.DOMEvent",
                            event,
                            DOWNLOADS_DOM_EVENT_MAX);
}

}  // namespace

MdDownloadsDOMHandler::MdDownloadsDOMHandler(
    content::DownloadManager* download_manager, content::WebUI* web_ui)
    : list_tracker_(download_manager, web_ui),
      item_selected_remove_(NULL),
      weak_ptr_factory_(this) {
  // Create our fileicon data source.
  profile_ = Profile::FromBrowserContext(download_manager->GetBrowserContext());
  content::URLDataSource::Add(profile_, new FileIconSource());

  check_timer_.reset(new base::RepeatingTimer());
  if (!check_timer_->IsRunning()) {
    check_timer_->Start(FROM_HERE,
                         base::TimeDelta::FromMilliseconds(kUpdatePeriodMs),
                         this, &MdDownloadsDOMHandler::CheckForRemovedFiles);
  }
}

MdDownloadsDOMHandler::~MdDownloadsDOMHandler() {
  check_timer_.reset();
  //FinalizeRemovals();
}

// MdDownloadsDOMHandler, public: ---------------------------------------------

void MdDownloadsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("getDownloads",
      base::Bind(&MdDownloadsDOMHandler::HandleGetDownloads,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("openFile",
      base::Bind(&MdDownloadsDOMHandler::HandleOpenFile,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("drag",
      base::Bind(&MdDownloadsDOMHandler::HandleDrag,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("saveDangerous",
      base::Bind(&MdDownloadsDOMHandler::HandleSaveDangerous,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("discardDangerous",
      base::Bind(&MdDownloadsDOMHandler::HandleDiscardDangerous,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("show",
      base::Bind(&MdDownloadsDOMHandler::HandleShow,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("pause",
      base::Bind(&MdDownloadsDOMHandler::HandlePause,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("resume",
      base::Bind(&MdDownloadsDOMHandler::HandleResume,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("remove",
      base::Bind(&MdDownloadsDOMHandler::HandleRemove,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("retry",
      base::Bind(&MdDownloadsDOMHandler::HandleRetry,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("undo",
      base::Bind(&MdDownloadsDOMHandler::HandleUndo,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("cancel",
      base::Bind(&MdDownloadsDOMHandler::HandleCancel,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("clearAll",
      base::Bind(&MdDownloadsDOMHandler::HandleClearAll,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("openDownloadsFolder",
      base::Bind(&MdDownloadsDOMHandler::HandleOpenDownloadsFolder,
                 weak_ptr_factory_.GetWeakPtr()));

  // append the new download webui messaages
  web_ui()->RegisterMessageCallback("newItem",
      base::Bind(&MdDownloadsDOMHandler::HandleNewItem,
                 weak_ptr_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback("openSetting",
      base::Bind(&MdDownloadsDOMHandler::HandleOpenSetting,
                 weak_ptr_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback("rightMenu",
      base::Bind(&MdDownloadsDOMHandler::HandleRightMenu,
                 weak_ptr_factory_.GetWeakPtr()));

  Observe(GetWebUIWebContents());
}

void MdDownloadsDOMHandler::OnJavascriptDisallowed() {
  list_tracker_.Stop();
  list_tracker_.Reset();
  CheckForRemovedFiles();
}

void MdDownloadsDOMHandler::RenderViewReused(content::RenderViewHost* render_view_host) {
 }

void MdDownloadsDOMHandler::RenderProcessGone(base::TerminationStatus status) {
  // TODO(dbeam): WebUI + WebUIMessageHandler should do this automatically.
  // http://crbug.com/610450
  DisallowJavascript();
}

void MdDownloadsDOMHandler::HandleGetDownloads(const base::ListValue* args) {
  AllowJavascript();

  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_GET_DOWNLOADS);

  bool terms_changed = list_tracker_.SetSearchTerms(*args);
  if (terms_changed)
    list_tracker_.Reset();

  list_tracker_.StartAndSendChunk();
}

void MdDownloadsDOMHandler::HandleOpenFile(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_OPEN_FILE);
  content::DownloadItem* file = GetDownloadByValue(args);
  if (file)
    file->OpenDownload();
}

void MdDownloadsDOMHandler::HandleDrag(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_DRAG);
  content::DownloadItem* file = GetDownloadByValue(args);
  if (!file)
    return;

  content::WebContents* web_contents = GetWebUIWebContents();
  // |web_contents| is only NULL in the test.
  if (!web_contents)
    return;

  if (file->GetState() != content::DownloadItem::COMPLETE)
    return;

  gfx::Image* icon = g_browser_process->icon_manager()->LookupIconFromFilepath(
      file->GetTargetFilePath(), IconLoader::NORMAL);
  gfx::NativeView view = web_contents->GetNativeView();
  {
    // Enable nested tasks during DnD, while |DragDownload()| blocks.
    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoop::current());
    DragDownloadItem(file, icon, view);
  }
}

void MdDownloadsDOMHandler::HandleSaveDangerous(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_SAVE_DANGEROUS);
  content::DownloadItem* file = GetDownloadByValue(args);
  SaveDownload(file);
}

void MdDownloadsDOMHandler::SaveDownload(
    content::DownloadItem* download) {
  if (!download)
    return;
  // If danger type is NOT DANGEROUS_FILE, chrome shows users a download danger
  // prompt.
  if (download->GetDangerType() !=
      content::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE) {
    ShowDangerPrompt(download);
  } else {
    // If danger type is DANGEROUS_FILE, chrome proceeds to keep this download
    // without showing download danger prompt.
    if (profile_) {
    #if defined(FULL_SAFE_BROWSING)
      PrefService* prefs = profile_->GetPrefs();
      if (!profile_->IsOffTheRecord() &&
          prefs->GetBoolean(prefs::kSafeBrowsingEnabled)) {
        DownloadDangerPrompt::SendSafeBrowsingDownloadReport(
            safe_browsing::ClientSafeBrowsingReportRequest::
                DANGEROUS_DOWNLOAD_RECOVERY,
            true, *download);
      }
      #endif
    }
    DangerPromptDone(download->GetId(), DownloadDangerPrompt::ACCEPT);
  }
}

void MdDownloadsDOMHandler::HandleDiscardDangerous(
    const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_DISCARD_DANGEROUS);
  RemoveDownloadInArgs(args);
}

void MdDownloadsDOMHandler::HandleShow(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_SHOW);
  content::DownloadItem* file = GetDownloadByValue(args);
  if (file)
    file->ShowDownloadInShell();
}

void MdDownloadsDOMHandler::HandlePause(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_PAUSE);
  content::DownloadItem* file = GetDownloadByValue(args);
  if (file)
    file->Pause();
}

void MdDownloadsDOMHandler::HandleResume(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_RESUME);
  content::DownloadItem* file = GetDownloadByValue(args);
  if (file)
    file->Resume();
}

void MdDownloadsDOMHandler::HandleRemove(const base::ListValue* args) {
  if (!IsDeletingHistoryAllowed())
    return;

  item_selected_remove_ = GetDownloadByValue(args);
  if (!item_selected_remove_)
    return;

  Browser* browser = chrome::GetDownloadManagerBrowser();
  if (browser && browser->window()) {
    BrowserView* browserview = static_cast<BrowserView*>(browser->window());
    gfx::NativeWindow parent = browserview->GetNativeWindow();

    // show the messagebox to confirm delete
    nfsbrowser::SimpleMessageBoxViews::InitParams params(parent,
                        nfsbrowser::MESSAGE_BOX_TYPE_QUESTION,
                        l10n_util::GetStringUTF16(IDS_DOWNLOAD_TASK_REMOVE_CONFIRM_TITLE),
                        l10n_util::GetStringUTF16(IDS_DOWNLOAD_TASK_REMOVE_CONFIRM_CONTENT));

    params.yes_text = l10n_util::GetStringUTF16(IDS_CLOSE_MULTI_TABS_OK);
    params.no_text = l10n_util::GetStringUTF16(IDS_CLOSE_MULTI_TABS_CANCEL);
    params.checkbox_text = l10n_util::GetStringUTF16(IDS_DOWNLOAD_TASK_REMOVE_CONFIRM_REMAINDER);
    params.callback = base::Bind(&MdDownloadsDOMHandler::RemoveConfirmed, base::Unretained(this));
    nfsbrowser::SimpleMessageBoxViews::ShowNfsMessageBox(params);
  }
}

void MdDownloadsDOMHandler::HandleRetry(const base::ListValue* args) {
  content::DownloadItem* file = GetDownloadByValue(args);
  if (file) {
      std::unique_ptr<content::DownloadCreateInfo> info(new content::DownloadCreateInfo);
      std::unique_ptr<content::ByteStreamReader> stream = NULL;
      //info->download_id = file->GetId();
      info->url_chain.push_back(file->GetURL());
      info->referrer_url = file->GetReferrerUrl();

      info->force_speedy = true;

      info->tab_url = file->GetTabUrl();
      info->method = file->GetMethod();
      info->pack_url = file->GetPackUrl();
      info->save_info->suggested_name = base::UTF8ToUTF16(file->GetSuggestedFilename());
      info->save_info->file_path = file->GetTargetFilePath();
      info->result = content::DOWNLOAD_INTERRUPT_REASON_NONE;
      info->start_time = base::Time::Now();

      content::DownloadUrlParameters::OnStartedCallback on_started;
      content::DownloadManager* manager = GetMainNotifierManager();
      manager->CreateDownload(std::move(info), std::move(stream), on_started);
      chrome::CloseDownloadNewTask(GetWebUIWebContents());

      DownloadVector downloads;
      downloads.push_back(file);
      MdDownloadsImpl::RemoveDownloads(downloads);
  }
}

void MdDownloadsDOMHandler::HandleUndo(const base::ListValue* args) {
  // TODO(dbeam): handle more than removed downloads someday?
  if (removals_.empty())
    return;

  const IdSet last_removed_ids = removals_.back();
  removals_.pop_back();

  const bool undoing_clear_all = last_removed_ids.size() > 1;
  if (undoing_clear_all) {
    list_tracker_.Reset();
    list_tracker_.Stop();
  }

  for (auto id : last_removed_ids) {
    content::DownloadItem* download = GetDownloadById(id);
    if (!download)
      continue;

    DownloadItemModel model(download);
    model.SetShouldShowInShelf(true);
    model.SetIsBeingRevived(true);

    download->UpdateObservers();

    model.SetIsBeingRevived(false);
  }

  if (undoing_clear_all)
    list_tracker_.StartAndSendChunk();
}

void MdDownloadsDOMHandler::HandleCancel(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_CANCEL);
  content::DownloadItem* file = GetDownloadByValue(args);
  if (file)
    file->Cancel(true);
}

void MdDownloadsDOMHandler::HandleClearAll(const base::ListValue* args) {
  if (!IsDeletingHistoryAllowed()) {
    // This should only be reached during tests.
    return;
  }

  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_CLEAR_ALL);

  list_tracker_.Reset();
  list_tracker_.Stop();

  DownloadVector downloads;
  if (GetMainNotifierManager())
    GetMainNotifierManager()->GetAllDownloads(&downloads);
  if (GetOriginalNotifierManager())
    GetOriginalNotifierManager()->GetAllDownloads(&downloads);

  DownloadVector complete_downloads;
  // clear the complete task only
  for (auto* download : downloads) {
    if (download->GetState() == content::DownloadItem::COMPLETE)  {
      complete_downloads.push_back(download);
    }
  }

  MdDownloadsImpl::RemoveDownloads(complete_downloads);

  list_tracker_.StartAndSendChunk();
}

void MdDownloadsImpl::RemoveDownloads(const DownloadVector& to_remove) {
  //IdSet ids;

  for (auto* download : to_remove) {
    if (download->IsDangerous()) {
      // Don't allow users to revive dangerous downloads; just nuke 'em.
      download->Remove();
      continue;
    }

    DownloadItemModel item_model(download);
    if (!item_model.ShouldShowInShelf() ||
        download->GetState() == content::DownloadItem::IN_PROGRESS) {
      continue;
    }

    item_model.SetShouldShowInShelf(false);
    //ids.insert(download->GetId());
    download->UpdateObservers();
    download->Remove();
  }

  //if (!ids.empty())
  //  removals_.push_back(ids);
}

void MdDownloadsDOMHandler::HandleOpenDownloadsFolder(
    const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_OPEN_FOLDER);
  content::DownloadManager* manager = GetMainNotifierManager();
  if (manager) {
    platform_util::OpenItem(
        Profile::FromBrowserContext(manager->GetBrowserContext()),
        DownloadPrefs::FromDownloadManager(manager)->DownloadPath(),
        platform_util::OPEN_FOLDER, platform_util::OpenOperationCallback());
  }
}

void MdDownloadsDOMHandler::HandleNewItem(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_NEWITEM);
  content::DownloadManager* manager = GetMainNotifierManager();
  Profile* profile = Profile::FromBrowserContext(
    manager->GetBrowserContext());
  if (profile) {
    chrome::ShowDownloadNewTask(profile);
  }
}

void MdDownloadsDOMHandler::HandleOpenSetting(const base::ListValue* args) {
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_OPENSETTING);
  content::WebContents* web_contents = MdDownloadsDOMHandler::GetWebUIWebContents();
  if (web_contents) {
    Browser* browser = chrome::FindLastActive();
    if(browser->is_type_simple_web())
    if (browser == NULL)  {
      content::DownloadManager* manager = GetMainNotifierManager();
      Profile* profile = Profile::FromBrowserContext(
          manager->GetBrowserContext());
      browser = new Browser(Browser::CreateParams(profile));
    }
    if (browser) {
      chrome::NavigateParams params(
          chrome::GetSingletonTabNavigateParams(browser, GURL(chrome::kChromeUIDownloadSettingsURL)));
      chrome::Navigate(&params);
    }
  }
}

void MdDownloadsDOMHandler::HandleRightMenu(const base::ListValue* args) {
  content::DownloadItem* item = GetDownloadByValue(args);
  if (item) {
    int point_x = -1;
    int point_y = -1;
    if (!args->GetInteger(1, &point_x) ||
        !args->GetInteger(2, &point_y)) {
      return;
    }

    content::DownloadManager* manager = GetMainNotifierManager();
    content::WebContents* web_contents = GetWebUIWebContents();
    manager->GetDelegate()->ShowDownloadRightMenu(item, web_contents, point_x, point_y);
  }
}

void MdDownloadsDOMHandler::RemoveConfirmed(nfsbrowser::MessageBoxResult result, bool checked) {
  if (result == nfsbrowser::MESSAGE_BOX_RESULT_NO ||
      !item_selected_remove_) {
    return;
  }

  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_REMOVE);
  item_selected_remove_->DeleteFileOnDisk(checked);
  content::DownloadItem::DownloadState state = item_selected_remove_->GetState();
  if (state == content::DownloadItem::IN_PROGRESS ||
      state == content::DownloadItem::PAUSING) {
    item_selected_remove_->Cancel(true);
  }

  // finally remove the selected download
  item_selected_remove_->Remove();
}

// MdDownloadsDOMHandler, private: --------------------------------------------

content::DownloadManager* MdDownloadsDOMHandler::GetMainNotifierManager()
    const {
  return list_tracker_.GetMainNotifierManager();
}

content::DownloadManager* MdDownloadsDOMHandler::GetOriginalNotifierManager()
    const {
  return list_tracker_.GetOriginalNotifierManager();
}

void MdDownloadsDOMHandler::FinalizeRemovals() {
  while (!removals_.empty()) {
    const IdSet remove = removals_.back();
    removals_.pop_back();

    for (const auto id : remove) {
      content::DownloadItem* download = GetDownloadById(id);
      if (download)
        download->Remove();
    }
  }
}

void MdDownloadsDOMHandler::ShowDangerPrompt(
    content::DownloadItem* dangerous_item) {
  DownloadDangerPrompt* danger_prompt = DownloadDangerPrompt::Create(
      dangerous_item,
      GetWebUIWebContents(),
      false,
      base::Bind(&MdDownloadsDOMHandler::DangerPromptDone,
                 weak_ptr_factory_.GetWeakPtr(), dangerous_item->GetId()));
  // danger_prompt will delete itself.
  DCHECK(danger_prompt);
}

void MdDownloadsDOMHandler::DangerPromptDone(
    int download_id, DownloadDangerPrompt::Action action) {
  if (action != DownloadDangerPrompt::ACCEPT)
    return;
  content::DownloadItem* item = NULL;
  if (GetMainNotifierManager())
    item = GetMainNotifierManager()->GetDownload(download_id);
  if (!item && GetOriginalNotifierManager())
    item = GetOriginalNotifierManager()->GetDownload(download_id);
  if (!item || item->IsDone())
    return;
  CountDownloadsDOMEvents(DOWNLOADS_DOM_EVENT_SAVE_DANGEROUS);
  item->ValidateDangerousDownload();
}

bool MdDownloadsDOMHandler::IsDeletingHistoryAllowed() {
  content::DownloadManager* manager = GetMainNotifierManager();
  return manager &&
         Profile::FromBrowserContext(manager->GetBrowserContext())->
             GetPrefs()->GetBoolean(prefs::kAllowDeletingBrowserHistory);
}

content::DownloadItem* MdDownloadsDOMHandler::GetDownloadByValue(
    const base::ListValue* args) {
  std::string download_id;
  if (!args->GetString(0, &download_id)) {
    NOTREACHED();
    return nullptr;
  }

  uint64_t id;
  if (!base::StringToUint64(download_id, &id)) {
    NOTREACHED();
    return nullptr;
  }
  
  return GetDownloadById(static_cast<uint32_t>(id));
}

content::DownloadItem* MdDownloadsDOMHandler::GetDownloadById(uint32_t id) {
  content::DownloadItem* item = NULL;
  if (GetMainNotifierManager())
    item = GetMainNotifierManager()->GetDownload(id);
  if (!item && GetOriginalNotifierManager())
    item = GetOriginalNotifierManager()->GetDownload(id);
  return item;
}

content::WebContents* MdDownloadsDOMHandler::GetWebUIWebContents() {
  return web_ui()->GetWebContents();
}

void MdDownloadsDOMHandler::CheckForRemovedFiles() {
  if (GetMainNotifierManager())
    GetMainNotifierManager()->CheckForHistoryFilesRemoval();
  if (GetOriginalNotifierManager())
    GetOriginalNotifierManager()->CheckForHistoryFilesRemoval();
}

void MdDownloadsDOMHandler::RemoveDownloadInArgs(const base::ListValue* args) {
  content::DownloadItem* file = GetDownloadByValue(args);
  if (!file)
    return;

  DownloadVector downloads;
  downloads.push_back(file);
  MdDownloadsImpl::RemoveDownloads(downloads);
}
