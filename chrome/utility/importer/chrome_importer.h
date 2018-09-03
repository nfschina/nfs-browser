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

#ifndef CHROME_UTILITY_IMPORTER_CHROME_IMPORTER_H_
#define CHROME_UTILITY_IMPORTER_CHROME_IMPORTER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/utility/importer/importer.h"
#include "components/bookmarks/browser/bookmark_model.h"

class GURL;

class ChromeImporter : public Importer
                     /*, public ChromeBookmarkFileReader*/ {
public:
  ChromeImporter();

  // static void DetectChromeProfiles(const std::string& locale,
  //                                 std::vector<importer::SourceProfile>* profiles);

  // Importer:
  void StartImport(const importer::SourceProfile& source_profile,
                     uint16_t items,
                     ImporterBridge* bridge) override;

protected:
  void HandleEntry(const std::string& category,
                   const base::DictionaryValue& entries);

private:
  ~ChromeImporter() override;

  void ImportBookmarks();

  void LoadFile(base::FilePath& file);

  // static void ReadProfiles(std::vector<importer::ChromeProfileInfo>* cp,
  //                  base::FilePath& profileDirectory);

  void AddBookmark(std::vector<base::string16>& current_folder,
                   const base::DictionaryValue& entries,
                   bool is_folder,
                   base::string16* item_name = NULL);

  bool DecodeNode(const base::DictionaryValue& value);
  bool DecodeChildren(const base::ListValue& child_value_list);

  const std::vector<ImportedBookmarkEntry>& Bookmarks() const { return bookmarks_; }

  void ClearBookmarks() { bookmarks_.clear(); current_folder_.clear(); }

  typedef std::map<int64_t, std::set<GURL>> FaviconMap;

  base::FilePath source_path_;
  base::FilePath profile_path_;
  base::FilePath bookmarks_path_;

#if defined(OS_POSIX)
  std::string locale_;
#endif

  std::vector<base::string16> current_folder_;
  std::vector<ImportedBookmarkEntry> bookmarks_;

  DISALLOW_COPY_AND_ASSIGN(ChromeImporter);
};  // class ChromeImporter

#endif  // CHROME_UTILITY_IMPORTER_CHROME_IMPORTER_H_
