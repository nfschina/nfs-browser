// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/md_downloads/md_downloads_new/md_downloads_dom_new_handler.h"

#include <algorithm>
#include <functional>
#if defined(OS_WIN)
#include <comdef.h>
#endif
#if !defined(OS_WIN)
#include <sys/statfs.h>
#endif

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/supports_user_data.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "base/files/file_util.h"
#include "net/base/filename_util.h"

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
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/webui/fileicon_source.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/browser/byte_stream.h"
#include "content/browser/download/download_create_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "net/base/filename_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/gfx/image/image.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "net/base/auth.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

using base::UserMetricsAction;
using content::BrowserThread;

#if defined(OS_POSIX)
#define STRING_LITERAL(x) x
typedef std::string string16;
#elif defined(OS_WIN)
#define STRING_LITERAL(x) L ## x
typedef std::wstring string16;
#endif  // OS_WIN

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

}  // namespace

MdDownloadsDOMNewHandler::MdDownloadsDOMNewHandler(
    content::DownloadManager* download_manager, content::WebUI* web_ui)
    : list_tracker_(download_manager, web_ui),
      weak_ptr_factory_(this) {
  // Create our fileicon data source.
  Profile* profile = Profile::FromBrowserContext(
      download_manager->GetBrowserContext());
  content::URLDataSource::Add(profile, new FileIconSource());
}

MdDownloadsDOMNewHandler::~MdDownloadsDOMNewHandler() {
}

// MdDownloadsDOMNewHandler, public: ---------------------------------------------
void MdDownloadsDOMNewHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("newDownloadLoad",
      base::Bind(&MdDownloadsDOMNewHandler::HandleNewDownloadLoad,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("newDownloadOk",
      base::Bind(&MdDownloadsDOMNewHandler::HandleNewDownloadOk,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("getDownloadName",
      base::Bind(&MdDownloadsDOMNewHandler::HandleGetDownloadName,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("getDiskSpace",
      base::Bind(&MdDownloadsDOMNewHandler::HandleGetDiskSpace,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("newDownloadCancel",
      base::Bind(&MdDownloadsDOMNewHandler::HandleNewDownloadCancel,
                 weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback("newDownloadBrowser",
      base::Bind(&MdDownloadsDOMNewHandler::HandleNewDownloadBrowser,
                 weak_ptr_factory_.GetWeakPtr()));
}

void MdDownloadsDOMNewHandler::RenderViewReused(
    content::RenderViewHost* render_view_host) {
}

void MdDownloadsDOMNewHandler::HandleNewDownloadLoad(const base::ListValue* args) {
  list_tracker_.OnLoad();
}

void MdDownloadsDOMNewHandler::HandleGetDiskSpace(const base::ListValue* args) {
  std::string download_path;
  if (!args->GetString(0, &download_path))
    return;
  #if defined(OS_WIN)
  double dwFreeDiskSpace = 0;

  ULARGE_INTEGER uiFreeBytesAvailableToCaller;
  ULARGE_INTEGER uiTotalNumberOfBytes;
  ULARGE_INTEGER uiTotalNumberOfFreeBytes;

  char drive[100];
  char dir[100];
  char fname[100];
  char ext[100];
  _splitpath( download_path.c_str(), drive, dir, fname, ext );
  _bstr_t path = drive;
  wchar_t* pchar = (wchar_t*)path;
  std::wstring result = pchar;
  if (GetDiskFreeSpaceEx(result.c_str(), &uiFreeBytesAvailableToCaller,
      &uiTotalNumberOfBytes,
      &uiTotalNumberOfFreeBytes))
  {
    dwFreeDiskSpace = ((double)(uiFreeBytesAvailableToCaller.QuadPart) / 1024 / 1024 /1024);
  }
  web_ui()->CallJavascriptFunctionUnsafe("newDownloadDiskSpaceGet", base::StringValue(base::DoubleToString(dwFreeDiskSpace)));

  #elif defined(OS_LINUX)
  struct statfs diskInfo;
  statfs(download_path.c_str(), &diskInfo);
  double freeDisk = (double)(diskInfo.f_bfree *  diskInfo.f_bsize)/(1024*1024*1024);
  web_ui()->CallJavascriptFunctionUnsafe("newDownloadDiskSpaceGet", base::StringValue(base::DoubleToString(freeDisk)));
  #endif
}

bool PathExistsInDownloads(base::FilePath target_path,
    DownloadQuery::DownloadVector* downloads) {
  DCHECK(downloads);

  // check if the target path in the downloads
  DownloadQuery::DownloadVector::iterator it;
  for (it = downloads->begin(); it != downloads->end(); ++it) {
    base::FilePath item_path = (*it)->GetTargetFilePath();
    if (item_path == target_path) {
      return true;
    }
  }

  return false;
}

void MdDownloadsDOMNewHandler::HandleGetDownloadName(const base::ListValue* args) {
  string16 download_url;
  string16 download_path;

  if (!args->GetString(0, &download_url) ||
       !args->GetString(1, &download_path))
    return;

  base::FilePath generated_filename = net::GenerateFileName(
    GURL(download_url),
    "",
    "",
    "",
    "",
    "");

  base::FilePath base_name = generated_filename.BaseName().RemoveExtension();
  string16 extension = generated_filename.Extension();
  if (extension.empty()) {
    if (GURL(download_url).SchemeIsData())
         extension = STRING_LITERAL("gif");
  }

  base::FilePath target_path = base::FilePath(download_path).Append(base_name).AddExtension(extension);
  int index = 1;
  bool previous_disallowed = base::ThreadRestrictions::SetIOAllowed(true);

  DownloadQuery::DownloadVector all_items;
  GetMainNotifierManager()->GetAllDownloads(&all_items);

  if (GetOriginalNotifierManager())
    GetOriginalNotifierManager()->GetAllDownloads(&all_items);

  while (base::PathExists(target_path) || PathExistsInDownloads(target_path, &all_items)) {
    #if defined(OS_POSIX)
    string16 serial_code = base::IntToString(index++);
    #elif defined(OS_WIN)
    string16 serial_code = base::IntToString16(index++);
    #endif
    base::FilePath new_base_name = base::FilePath(base_name.value()
      + STRING_LITERAL("(") + serial_code + STRING_LITERAL(")"));
    generated_filename = new_base_name.AddExtension(extension);
    target_path = base::FilePath(download_path).Append(new_base_name).AddExtension(extension);
  }

  base::ThreadRestrictions::SetIOAllowed(previous_disallowed);

  web_ui()->CallJavascriptFunctionUnsafe("newDownloadFilenameGet", base::StringValue(generated_filename.value()));
}

void MdDownloadsDOMNewHandler::HandleNewDownloadOk(const base::ListValue* args) {
  std::string download_url;
  string16 download_name;
  base::string16 download_name16;
  string16 download_path;
  base::string16 username;
  base::string16 password;
  if (!args->GetString(0, &download_url) ||
      !args->GetString(1, &download_name) ||
      !args->GetString(1, &download_name16) ||
      !args->GetString(2, &download_path) ||
      !args->GetString(3, &username) ||
      !args->GetString(4, &password)) {
   DCHECK(false);
  }

  // check the download parameters before dowloading
  if (download_url.empty())
  {
     web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
       base::StringValue("url"),
	   //base::StringValue(STRING_LITERAL("网址为空, 请输入!")));
       base::StringValue(l10n_util::GetStringUTF16(IDS_WEBSITE_NONE_REENTER)));
     return;
  }
  else if (!GURL(download_url).is_valid())
  {
     web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
       base::StringValue("url"),
       //base::StringValue(STRING_LITERAL("网址URL无效, 请重新输入!")));
       base::StringValue(l10n_util::GetStringUTF16(IDS_WEBSITE_INVALID_REENTER)));
     return;
  }
  else if (download_name.empty())
  {
     web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
       base::StringValue("name"),
       //base::StringValue(STRING_LITERAL("文件名为空, 请输入!")));
       base::StringValue(l10n_util::GetStringUTF16(IDS_FILENAME_NONE_REENTER)));
     return;
  }
  #if defined(OS_LINUX)
  else if (download_name.length() > 255)
  {
     web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
       base::StringValue("name"),
       //base::StringValue(STRING_LITERAL("文件名过长, 请重新输入!")));
       base::StringValue(l10n_util::GetStringUTF16(IDS_FILENAME_TOOLONG_REENTER)));
     return;
  }
  #endif
  else if (download_path.empty())
  {
     web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
       base::StringValue("path"),
       //base::StringValue(STRING_LITERAL("下载路径为空, 请选择路径!")));
       base::StringValue(l10n_util::GetStringUTF16(IDS_DOWNLOAD_PATH_NONE_REENTER)));
     return;
  }
  else
  {
     //Check file name exists
     bool previous_disallowed = base::ThreadRestrictions::SetIOAllowed(true);
     if(base::PathExists(base::FilePath(download_path).Append(download_name)))
     {
        base::ThreadRestrictions::SetIOAllowed(previous_disallowed);
        web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
          base::StringValue("name"),
          //base::StringValue(STRING_LITERAL("该文件已存在, 请修改!")));
          base::StringValue(l10n_util::GetStringUTF16(IDS_FILE_EXIST_REENTER)));
        return;
     }

     base::ThreadRestrictions::SetIOAllowed(previous_disallowed);
  }

  // the download parameters are checked all right, begin to download now
  web_ui()->CallJavascriptFunctionUnsafe("newDownloadParameterCheckout",
    base::StringValue("OK"),
    base::StringValue(""));

  std::unique_ptr<content::DownloadCreateInfo> info(new content::DownloadCreateInfo);
  std::unique_ptr<content::ByteStreamReader> stream = NULL;
  info->url_chain.push_back(GURL(download_url));
  info->save_info->suggested_name = download_name16;
  info->save_info->file_path = base::FilePath(download_path).Append(download_name);

  if (list_tracker_.GetCreateInfo()) {
    info->referrer_url = list_tracker_.GetCreateInfo()->referrer_url;
    info->tab_url = list_tracker_.GetCreateInfo()->tab_url;
    info->method = list_tracker_.GetCreateInfo()->method;
    info->pack_url = list_tracker_.GetCreateInfo()->pack_url;
  }

  info->result = content::DOWNLOAD_INTERRUPT_REASON_NONE;
  info->start_time = base::Time::Now();

  info->auth_credential = net::AuthCredentials(username, password);
  info->force_speedy = true;

  content::DownloadUrlParameters::OnStartedCallback on_started;
  content::DownloadManager* manager = GetMainNotifierManager();
  manager->CreateDownload(std::move(info), std::move(stream), on_started);
  chrome::CloseDownloadNewTask(GetWebUIWebContents());
}

void MdDownloadsDOMNewHandler::HandleNewDownloadCancel(const base::ListValue* args) {
  chrome::CloseDownloadNewTask(GetWebUIWebContents());
}

void MdDownloadsDOMNewHandler::HandleNewDownloadBrowser(const base::ListValue* args) {
  content::WebContents* web_contents = GetWebUIWebContents();
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, new ChromeSelectFilePolicy(web_contents));

  base::FilePath suggested_path;
  ui::SelectFileDialog::FileTypeInfo file_type_info;
  // Platform file pickers, notably on Mac and Windows, tend to break
  // with double extensions like .tar.gz, so only pass in normal ones.
  base::FilePath::StringType extension = suggested_path.FinalExtension();
  if (!extension.empty()) {
    extension.erase(extension.begin());  // drop the .
    file_type_info.extensions.resize(1);
    file_type_info.extensions[0].push_back(extension);
  }

  file_type_info.include_all_files = true;
  file_type_info.allowed_paths =
      ui::SelectFileDialog::FileTypeInfo::NATIVE_OR_DRIVE_PATH;
  gfx::NativeWindow owning_window = web_contents ?
      platform_util::GetTopLevel(web_contents->GetNativeView()) : NULL;

  select_file_dialog_->SelectFile(ui::SelectFileDialog::SELECT_FOLDER,
                                  base::string16(),
                                  suggested_path,
                                  &file_type_info,
                                  0,
                                  base::FilePath::StringType(),
                                  owning_window,
                                  NULL);
}

void MdDownloadsDOMNewHandler::FileSelected(const base::FilePath& path,
                                            int index,
                                            void* params) {
  web_ui()->CallJavascriptFunctionUnsafe("browserSelected", base::StringValue((path.value())));
  Profile* profile = Profile::FromBrowserContext(
    GetMainNotifierManager()->GetBrowserContext());
  if (profile) {
    profile->set_download_selected_directory(path);
  }
}

void MdDownloadsDOMNewHandler::FileSelectionCanceled(void* params) {
  //LOG(INFO) << "FileSelectionCanceled";
}

// MdDownloadsDOMNewHandler, private: --------------------------------------------

content::DownloadManager* MdDownloadsDOMNewHandler::GetMainNotifierManager()
    const {
  return list_tracker_.GetMainNotifierManager();
}

content::DownloadManager* MdDownloadsDOMNewHandler::GetOriginalNotifierManager()
    const {
  return list_tracker_.GetOriginalNotifierManager();
}

content::WebContents* MdDownloadsDOMNewHandler::GetWebUIWebContents() {
  return web_ui()->GetWebContents();
}
