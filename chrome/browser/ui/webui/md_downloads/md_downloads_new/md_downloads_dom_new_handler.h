// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_DOM_NEW_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_DOM_NEW_HANDLER_H_

#include <stdint.h>

#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/download/download_danger_prompt.h"
#include "chrome/browser/ui/webui/md_downloads/md_downloads_new/downloads_list_new_tracker.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace base {
class ListValue;
}

namespace content {
class DownloadItem;
class DownloadManager;
class RenderViewHost;
class WebContents;
class WebUI;
}

// The handler for Javascript messages related to the "downloads" view,
// also observes changes to the download manager.
class MdDownloadsDOMNewHandler : public content::WebUIMessageHandler,
                                 public ui::SelectFileDialog::Listener {
 public:
  MdDownloadsDOMNewHandler(content::DownloadManager* download_manager,
                           content::WebUI* web_ui);
  ~MdDownloadsDOMNewHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  void RenderViewReused(content::RenderViewHost* render_view_host);

  // Callback for the "openSetting" message - enter into setting page.
  void HandleNewDownloadLoad(const base::ListValue* args);
  void HandleNewDownloadOk(const base::ListValue* args);
  void HandleGetDownloadName(const base::ListValue* args);
  void HandleGetDiskSpace(const base::ListValue* args);
  void HandleNewDownloadCancel(const base::ListValue* args);
  void HandleNewDownloadBrowser(const base::ListValue* args);

  // SelectFileDialog::Listener implementation.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;

  void FileSelectionCanceled(void* params) override;

 protected:
  // These methods are for mocking so that most of this class does not actually
  // depend on WebUI. The other methods that depend on WebUI are
  // RegisterMessages() and HandleDrag().
  virtual content::WebContents* GetWebUIWebContents();

 private:

  // Convenience method to call |main_notifier_->GetManager()| while
  // null-checking |main_notifier_|.
  content::DownloadManager* GetMainNotifierManager() const;

  // Convenience method to call |original_notifier_->GetManager()| while
  // null-checking |original_notifier_|.
  content::DownloadManager* GetOriginalNotifierManager() const;

  DownloadsListNewTracker list_tracker_;

  // For managing select file dialogs.
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;

  base::WeakPtrFactory<MdDownloadsDOMNewHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MdDownloadsDOMNewHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_DOM_NEW_HANDLER_H_
