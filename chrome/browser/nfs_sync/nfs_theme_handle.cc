#include "chrome/browser/nfs_sync/nfs_theme_handle.h"

#include "base/path_service.h" //wdq
#include "base/strings/utf_string_conversions.h" //wdq
#include "base/files/file_util.h" //wdq
#include "chrome/browser/extensions/crx_installer.h" //wdq
#include "chrome/browser/extensions/extension_service.h" //wdq
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_system.h"
#include "third_party/curl/src/include/curl/curl.h" //wdq
#include "third_party/curl/src/include/curl/easy.h" //wdq

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

using extensions::Extension;

namespace {
const char KSkinOriginUrl[] = "http://124.16.141.197:3000/themes/";
const char kDefaultThemeID[] = "default";
}


namespace nfs_sync {

NfsThemeHandle::NfsThemeHandle(Profile* profile)
  : res_(CURLE_OK)
  , profile_(profile) {

}

NfsThemeHandle::~NfsThemeHandle() {
}


void NfsThemeHandle::ThemeLoad(std::string themeID) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  //themeID = std::string("depaceignponiakmpaleilbdigbbgkdn.crx");
  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&NfsThemeHandle::DownloadTheme, base::Unretained(this), themeID),
      base::Bind(&NfsThemeHandle::ThemeInstall, base::Unretained(this), themeID));
}

void NfsThemeHandle::DownloadTheme(std::string themeID) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  if(themeID.empty()) {
    crx_path_ = kDefaultThemeID;
    return;
  }
  std::string url = KSkinOriginUrl;
  url.append(themeID);
  url.append("/");
  url.append(themeID);
  url.append(".crx");

  base::FilePath temp_dir;
  if (!PathService::Get(base::DIR_TEMP, &temp_dir)) {
    res_ = CURLE_FAILED_INIT;
    return;
  }

  #if defined(OS_WIN)
    StringType theme_id = base::UTF8ToWide(themeID);
  #elif defined(OS_LINUX)
    StringType theme_id = themeID;
  #endif

  base::FilePath download_path = temp_dir.Append(theme_id);
  #if defined(OS_WIN)
    std::string download_path_utf8 = base::WideToUTF8(download_path.value());
  #elif defined(OS_LINUX)
    std::string download_path_utf8 = download_path.value();
  #endif
  crx_path_ = download_path_utf8;
  CURL *curl;
  FILE *fp;
  /*   调用curl_global_init()初始化libcurl  */

  res_ = curl_global_init(CURL_GLOBAL_ALL);

  /*  调用curl_easy_init()函数得到 easy interface型指针  */
  curl = curl_easy_init();
  fp = fopen(download_path_utf8.c_str(), "wb");
  if (!fp) {
    res_ = CURLE_FAILED_INIT;
    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return ;
  }

  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FILE, fp);
  res_ = curl_easy_perform(curl);

  if (CURLE_OK == res_) {
    fclose(fp);
  } else {
    fclose(fp);
  }

  /* always cleanup */
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void NfsThemeHandle::ThemeInstall(std::string themeID) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (themeID.empty()) {
    content::NotificationService* service =
        content::NotificationService::current();
    service->Notify(chrome::NOTIFICATION_THEME_GALLERY_CHANGE,
                  content::Source<NfsThemeHandle>(this),
                  content::Details<std::string>(&crx_path_));
    return;
  }


  if (res_ != CURLE_OK) {
    return;
  }

  ExtensionService* extension_service = extensions::ExtensionSystem::Get(profile_)->extension_service();
  if (extension_service == NULL) {
    LOG(ERROR) << "extension_service is NULL";
    return;
  }

  scoped_refptr<extensions::CrxInstaller> installer =
      extensions::CrxInstaller::CreateSilent(extension_service);

  installer->set_allow_silent_install(true);
  installer->set_install_immediately(true);

  #if defined(OS_WIN)
    const base::FilePath crx_path= base::FilePath(base::UTF8ToWide(crx_path_));
  #elif defined(OS_LINUX)
    const base::FilePath crx_path= base::FilePath(crx_path_);
  #endif

  installer->InstallCrx(crx_path);
}

} //namespace nfs_sync

