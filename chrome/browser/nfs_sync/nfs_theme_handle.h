#ifndef COMPONENTS_NFS_SYNC_NFS_THEME_HANDLE1111_H_
#define COMPONENTS_NFS_SYNC_NFS_THEME_HANDLE1111_H_

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "third_party/curl/src/include/curl/curl.h"

#include <string>

namespace nfs_sync{

class NfsThemeHandle {
public:

  explicit NfsThemeHandle(Profile* profile);
  void ThemeLoad(std::string themeID);
  void DownloadTheme(std::string themeID);
  void ThemeInstall(std::string themeID);
  ~NfsThemeHandle();

private:
  std::string crx_path_;
  CURLcode res_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NfsThemeHandle);
};

}

#endif  // COMPONENTS_NFS_SYNC_NFS_THEME_HANDLE_H_
