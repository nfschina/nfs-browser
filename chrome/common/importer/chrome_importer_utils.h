#ifndef CHROME_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_
#define CHROME_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/importer/importer_type.h"
#include "chrome/common/importer/importer_data_types.h"

namespace base {
  class DictionaryValue;
  class FilePath;
}

// Returns the path to the Chrome profile.
base::FilePath GetChromeProfileDir();
void ReadChromeProfiles(std::vector<importer::ChromeProfileInfo> *cp,
                        base::FilePath profileDirectory);


#endif  // CHROME_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_
