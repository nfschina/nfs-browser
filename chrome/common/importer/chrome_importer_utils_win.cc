#include "chrome/common/importer/chrome_importer_utils.h"

#include "base/base_paths_win.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

base::FilePath GetChromeProfileDir() {
  base::FilePath chrome_profile_path, app_data_path;

  PathService::Get(base::DIR_LOCAL_APP_DATA, &app_data_path);
  if (!app_data_path.empty()) {
    chrome_profile_path = app_data_path.AppendASCII("Google\\Chrome\\User Data");
  }
  if (base::PathExists(chrome_profile_path))
    return chrome_profile_path;

  return base::FilePath();
}
