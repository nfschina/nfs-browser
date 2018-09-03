#include "chrome/common/importer/chrome_importer_utils.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

base::FilePath GetChromeProfileDir() {
  base::FilePath chrome_profile_path, home;

  PathService::Get(base::DIR_HOME, &home);
  if (!home.empty()) {
    chrome_profile_path = home.Append(".config").Append("google-chrome");
  }
  if (base::PathExists(chrome_profile_path))
    return chrome_profile_path;

  return base::FilePath();
}
