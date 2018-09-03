#include "chrome/browser/notifications/notification_service.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/startup/check_update_infobar_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context_getter.h"

#if defined(OS_WIN)
#include <windows.h>
#include "Shellapi.h"
#elif defined(OS_LINUX)
#include <unistd.h>
#endif

#if defined(OS_WIN)
  typedef std::wstring StringType;
  const wchar_t kUpdaterName[] = L"nfs-browser-up.exe";
  const wchar_t kNewLauncherName[] = L"new_nfs-browser.exe";
#elif defined(OS_LINUX)
  typedef std::string StringType;
  const char kUpdaterName[] = "nfs-browser-up";
  const char kNewLauncherName[] = "new_nfs-browser";
#endif

const char kArch[] =
#if defined(__amd64__) || defined(_WIN64)
    "x64";
#elif defined(__i386__) || defined(_WIN32)
    "x86";
#else
#error "unknown arch"
#endif

const char kType[] = "NFSBrowser";
const char kUpdateURL[] = "http://180.167.10.100/update/update.php";

namespace {

void AutoUpdate(base::FilePath updater_exe_path,
                std::string url,
                std::string latest_version) {
  #if defined(OS_WIN)
    StringType url_arg = L"--url=" + base::UTF8ToWide(url) +
                         L" --version=" + base::UTF8ToWide(latest_version);
    ShellExecute(NULL, NULL, updater_exe_path.value().c_str(),
                 url_arg.c_str(), NULL, SW_SHOWNORMAL);
  #else
    StringType url_arg = "--url=" + url;
    StringType version_arg = "--version=" + latest_version;

    pid_t pid = fork();
    if (0 == pid) {
      char** args = new char* [4];

      args[0] = new char[updater_exe_path.value().size() + 1];
      strcpy(args[0], updater_exe_path.value().c_str());
      args[1] = new char[url_arg.size() + 1];
      strcpy(args[1], url_arg.c_str());
      args[2] = new char[version_arg.size() + 1];
      strcpy(args[2], version_arg.c_str());
      args[3] = (char*)0;

      execv(updater_exe_path.value().c_str(), args);

      for (int i = 0; i < 4; ++i) {
        delete[] args[i];
      }
      delete[] args;
    } else if (pid < 0) {
      LOG(FATAL) << "Failed to fork!";
    }
  #endif
}

}

// Limits the amount of data that will be buffered from the server's response.
class SizeLimitingStringWriter : public net::URLFetcherStringWriter {
 public:
  explicit SizeLimitingStringWriter(size_t limit) : limit_(limit) {}

  int Write(net::IOBuffer* buffer,
            int num_bytes,
            const net::CompletionCallback& callback) override {
    if (data().length() + num_bytes > limit_) {
      return net::ERR_FILE_TOO_BIG;
    }
    return net::URLFetcherStringWriter::Write(buffer, num_bytes, callback);
  }

 private:
  size_t limit_;
};

NotificationService::NotificationService(Profile* profile)
    : max_response_size_(1024)
    , profile_(profile) {
  DCHECK(profile_);
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  if (!profile_->GetPrefs()->GetBoolean(prefs::kAutoUpdateToLatest)) {
    return;
  }

  std::string scheme = "http://";
  std::string path = "/update/update.php";
  std::string query =
      base::StringPrintf("?os=%s&version=%s&arch=%s&type=%s",
                         base::SysInfo::GetSystemName().c_str(),
                         version_info::GetVersionNumber().c_str(),
                         kArch, kType);

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  DCHECK(command_line);
  if (!command_line || !command_line->HasSwitch(switches::kUpdateURL)) {
    url_ = kUpdateURL + query;
  } else {
    std::string ip = command_line->GetSwitchValueASCII(switches::kUpdateURL);
    if (ip.empty()) {
      url_ = kUpdateURL + query;
    } else {
      url_ = scheme + ip + path + query;
    }
  }

  getter_ = std::move(profile_->GetRequestContext());

  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(3),
               this, &NotificationService::CheckForUpdate);
}

NotificationService::~NotificationService() {
}

void NotificationService::CheckForUpdate() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // If new_nfsbrowser is exist，don't check update.
  base::FilePath module_path;
  if (!PathService::Get(base::DIR_MODULE, &module_path)) {
    return;
  }
  base::FilePath new_launcher_path =
      module_path.DirName().Append(kNewLauncherName);
  const bool io_allowed = base::ThreadRestrictions::SetIOAllowed(true);
  if (base::PathExists(new_launcher_path)) {
    timer_.Stop();
    base::ThreadRestrictions::SetIOAllowed(io_allowed);
    return;
  }
  base::ThreadRestrictions::SetIOAllowed(io_allowed);

  // This cancels any outstanding fetch.
  time_fetcher_ = net::URLFetcher::Create(
      GURL(url_), net::URLFetcher::GET, this);
  if (!time_fetcher_) {
    LOG(ERROR) << "tried to make fetch happen; failed";
    return;
  }
  time_fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherResponseWriter>(
          new SizeLimitingStringWriter(max_response_size_)));
  DCHECK(getter_);
  time_fetcher_->SetRequestContext(getter_.get());
  // Not expecting any cookies, but just in case.
  time_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                              net::LOAD_DO_NOT_SAVE_COOKIES |
                              net::LOAD_DO_NOT_SEND_COOKIES |
                              net::LOAD_DO_NOT_SEND_AUTH_DATA);

  time_fetcher_->Start();
}

void NotificationService::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(time_fetcher_);
  DCHECK_EQ(source, time_fetcher_.get());

  if (HandleResponse()) {

  } else {

  }

  timer_.Stop();
}

bool NotificationService::HandleResponse() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (time_fetcher_->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      time_fetcher_->GetResponseCode() != 200) {
    return false;
  }

  std::string response_body;
  if (!time_fetcher_->GetResponseAsString(&response_body)) {
    return false;
  }

  std::unique_ptr<base::Value> value = base::JSONReader::Read(response_body);
  if (!value) {
    return false;
  }
  const base::DictionaryValue* dict;
  if (!value->GetAsDictionary(&dict) || !dict) {
    return false;
  }

  bool is_latest_version;
  std::string error;
  if (!dict->GetBoolean("isLatest", &is_latest_version) ||
      !dict->GetString("error", &error)) {
    return false;
  }
  if (is_latest_version == true || !error.empty()) {
    return false;
  }

  std::string latest_version, name, url;
  if (!dict->GetString("version", &latest_version) ||
      !dict->GetString("name", &name) ||
      !dict->GetString("url", &url)) {
    return false;
  }

  /*
   * mode == 0: updated quietly
   * mode == 1: updated by user
  */
  int mode = 1;
  if (!dict->HasKey("mode") || !dict->GetInteger("mode", &mode)) {
    mode = 1;
  }

  if (AUTO_UPDATE == mode) {
    base::FilePath path;
    base::FilePath updater_exe_path;

    PathService::Get(base::DIR_EXE, &path);
    #if defined(OS_WIN)
      updater_exe_path = path.Append(kUpdaterName);
    #else
      base::FilePath browser_path;
      browser_path = path.DirName();
      updater_exe_path = browser_path.Append(kUpdaterName);
    #endif

    const bool io_allowed = base::ThreadRestrictions::SetIOAllowed(true);
    if (!base::PathExists(updater_exe_path)) {
      base::ThreadRestrictions::SetIOAllowed(io_allowed);
      return false;
    }
    base::ThreadRestrictions::SetIOAllowed(io_allowed);

    AutoUpdate(updater_exe_path, url, latest_version);
  } else if (MANUAL_UPDATE == mode) {
    chrome::CheckUpdateInfoBarDelegate::Create(
        InfoBarService::FromWebContents(
            chrome::FindLastActiveWithProfile(profile_)->
                tab_strip_model()->GetActiveWebContents()),
            profile_, latest_version, name , url);
  } else {
    return false;
  }

  return true;
}
