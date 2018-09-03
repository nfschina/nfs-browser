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

#ifndef CHROME_BROWSER_UI_WEBUI_UPDATE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_UPDATE_HANDLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "third_party/curl/src/include/curl/curl.h"

#include <string>

namespace net {
class URLRequestContextGetter;
}

class Browser;
class Profile;

// Handles actions on Welcome page.
class UpdateHandler : public content::WebUIMessageHandler {
 public:
#if defined(OS_POSIX)
  // On most platforms, native pathnames are char arrays, and the encoding
  // may or may not be specified.  On Mac OS X, native pathnames are encoded
  // in UTF-8.
  typedef std::string StringType;
#elif defined(OS_WIN)
  // On Windows, for Unicode-aware applications, native pathnames are wchar_t
  // arrays encoded in UTF-16.
  typedef std::wstring StringType;
#endif  // OS_WIN

  explicit UpdateHandler(content::WebUI* web_ui);
  ~UpdateHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  void DownloadFile();

 private:
  void HandleUpdateNfs(const base::ListValue* args);
  void HandleInstallUpdate(const base::ListValue* args);
  void HandleCloseDialog(const base::ListValue* args);
  void HandlePackageExist(const base::ListValue* args);
  void DownloadFileCallback();
  base::CancelableTaskTracker::TaskId StartDownload(base::CancelableTaskTracker* tracker);

  std::string url_;
  CURLcode res_;

  base::CancelableTaskTracker cancelable_task_tracker_;

  base::WeakPtrFactory<UpdateHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UpdateHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_UPDATE_HANDLER_H_
