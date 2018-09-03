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

#include "chrome/browser/ui/webui/update_nfs/update_handle.h"

#include "base/environment.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/profile_chooser_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_system.h"
#include "net/url_request/url_request_context.h"
#include "chrome/browser/browser_process.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/curl/src/include/curl/curl.h"
#include "third_party/curl/src/include/curl/easy.h"
#include "chrome/browser/ui/views/web_content_dialog_view.h"

#if defined(OS_WIN)
#include <windows.h>
#include "Shellapi.h"
#else
#include <unistd.h>
#endif

using content::BrowserThread;
using extensions::Extension;

namespace {

UpdateHandler::StringType GenerateRandomName() {
  const char table[64] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                           'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                           'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                           'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
                           'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                           'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                           'y', 'z', '_', '-'};
  char str[17];
  memset(str, '\0', 17);

  srand((unsigned)time(0));
  for (int i = 0; i < 16; ++i) {
    str[i] = table[rand() % 64];
  }

  #if defined(OS_WIN)
    return base::UTF8ToWide(str);
  #else
    return UpdateHandler::StringType(str);
  #endif
}

}

UpdateHandler::UpdateHandler(content::WebUI* web_ui):
    res_(CURLE_OK),
    weak_factory_(this) {
}

UpdateHandler::~UpdateHandler() {
  cancelable_task_tracker_.TryCancelAll();
}

void UpdateHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "updateNfs", base::Bind(&UpdateHandler::HandleUpdateNfs,
                                         base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "installUpdate", base::Bind(&UpdateHandler::HandleInstallUpdate,
                                         base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "closeDialog", base::Bind(&UpdateHandler::HandleCloseDialog,
                                         base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "packageExist", base::Bind(&UpdateHandler::HandlePackageExist,
                                         base::Unretained(this)));
}

void UpdateHandler::HandleCloseDialog(const base::ListValue* args) {
    nfsbrowser::WebContentDialogView::Hide(web_ui()->GetWebContents());
}

void UpdateHandler::HandleInstallUpdate(const base::ListValue* args) {
  nfsbrowser::WebContentDialogView::Hide(web_ui()->GetWebContents());

  base::FilePath temp_dir;
  if (!PathService::Get(base::DIR_TEMP, &temp_dir)) {
    return;
  }
  #if defined(OS_WIN)
    base::FilePath package_path = temp_dir.Append(L"temp_update_package.exe");
  #elif defined(OS_LINUX)
    base::FilePath package_path = temp_dir.Append("temp_update_package.deb");
  #endif

    const bool io_allowed = base::ThreadRestrictions::SetIOAllowed(true);
    if (!base::PathExists(package_path)) {
      base::ThreadRestrictions::SetIOAllowed(io_allowed);
      return ;
    }
    base::ThreadRestrictions::SetIOAllowed(io_allowed);

  #if defined(OS_WIN)

    SHELLEXECUTEINFO ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = L"runas";
    ShExecInfo.lpFile = package_path.value().c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ShellExecuteEx(&ShExecInfo);
  #else
    char* value = getenv("XDG_CURRENT_DESKTOP");
    std::string cmd;
    if (!value) {
      return;
    } else {
      if (strncmp(value, "KDE", 3) == 0) {
        cmd = "kdesu dpkg -i " + package_path.value();
      } else if (strncmp(value, "XFCE", 4) == 0) {
        cmd = "xsu dpkg -i " + package_path.value();
      } else {
        cmd = "pkexec dpkg -i " + package_path.value();
      }
    }

    printf("value=%s\n", value);
    pid_t pid = fork();
    if(pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)0);
    }
  #endif
}

void UpdateHandler::HandleUpdateNfs(const base::ListValue* args) {
  std::string url_string;
  if (!args->GetString(1, &url_string)) {
    web_ui()->CallJavascriptFunctionUnsafe("update.downloadFailed");
    return ;
  }

  url_ = url_string;
  if (url_.empty()) {
    web_ui()->CallJavascriptFunctionUnsafe("update.downloadFailed");
    return ;
  }

  StartDownload(&cancelable_task_tracker_);
}

void UpdateHandler::HandlePackageExist(const base::ListValue* args) {
  base::FilePath tmp_path;
  if (!PathService::Get(base::DIR_TEMP, &tmp_path)) {
    web_ui()->CallJavascriptFunctionUnsafe("update.packageExist",
                                           base::FundamentalValue(false));
    return ;
  }

#if defined(OS_WIN)
  base::FilePath download_path = tmp_path.Append(L"temp_update_package.exe");
#elif defined(OS_LINUX)
  base::FilePath download_path = tmp_path.Append("temp_update_package.deb");
#endif

  const bool io_allowed = base::ThreadRestrictions::SetIOAllowed(true);

  if (base::PathExists(download_path)) {
    web_ui()->CallJavascriptFunctionUnsafe("update.packageExist",
                                           base::FundamentalValue(true));
    return ;
  } else {
    web_ui()->CallJavascriptFunctionUnsafe("update.packageExist",
                                           base::FundamentalValue(false));
    return ;
  }

  base::ThreadRestrictions::SetIOAllowed(io_allowed);
}

void UpdateHandler::DownloadFile() {
  DCHECK(!url_.empty());
  if (url_.empty()) {
    res_ = CURLE_FAILED_INIT;
    return;
  }

  base::FilePath temp_dir;
  if (!PathService::Get(base::DIR_TEMP, &temp_dir)) {
    res_ = CURLE_FAILED_INIT;
    return;
  }

  StringType temp_file_name = GenerateRandomName();
  base::FilePath download_path = temp_dir.Append(temp_file_name);
  #if defined(OS_WIN)
    std::string download_path_utf8 = base::WideToUTF8(download_path.value());
  #elif defined(OS_LINUX)
    std::string download_path_utf8 = download_path.value();
  #endif

  CURL *curl;
  FILE *fp;

  /*   调用curl_global_init()初始化libcurl  */
  res_ = curl_global_init(CURL_GLOBAL_ALL);

  /*  调用curl_easy_init()函数得到 easy interface型指针  */
  curl = curl_easy_init();
  fp = fopen(download_path_utf8.c_str(), "wb");
  if (!fp) {
    res_ = CURLE_FAILED_INIT;
    return ;
  }

  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl, CURLOPT_FILE, fp);
  res_ = curl_easy_perform(curl);

  if (CURLE_OK == res_) {
    fclose(fp);

  #if defined(OS_WIN)
    base::FilePath package_path = temp_dir.Append(L"temp_update_package.exe");
  #elif defined(OS_LINUX)
    base::FilePath package_path = temp_dir.Append("temp_update_package.deb");
  #endif

    base::File::Error error;
    if (!base::ReplaceFile(download_path, package_path, &error)) {
      base::DeleteFile(download_path, false);
      res_ = CURLE_FAILED_INIT;
    }
  } else {
    fclose(fp);
  }

  /* always cleanup */
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void UpdateHandler::DownloadFileCallback() {
  if (CURLE_OK == res_) {
    web_ui()->CallJavascriptFunctionUnsafe("update.downloadComplete");
  } else {
    LOG(ERROR) << "download fail";
    web_ui()->CallJavascriptFunctionUnsafe("update.downloadFailed");
  }
}

base::CancelableTaskTracker::TaskId UpdateHandler::StartDownload(
    base::CancelableTaskTracker* tracker) {
  return tracker->PostTaskAndReply(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::FILE).get(),
      FROM_HERE,
      base::Bind(&UpdateHandler::DownloadFile,
                  base::Unretained(this)),
      base::Bind(&UpdateHandler::DownloadFileCallback,
                  weak_factory_.GetWeakPtr()));
}
