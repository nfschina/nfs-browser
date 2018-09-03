#include "chrome/common/importer/chrome_importer_utils.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"

void ReadChromeProfiles(std::vector<importer::ChromeProfileInfo> *cp,
                  base::FilePath profileDirectory) {
  base::FilePath profile_filename = profileDirectory.AppendASCII("Local State");
  if (!base::PathExists(profile_filename))
    return;

  std::string input;
  base::ReadFileToString(profile_filename, &input);

  base::JSONReader reader;
  std::unique_ptr<base::Value> root(reader.ReadToValue(input));
  if (!root || !root.get()) {
    return;
  }

  base::DictionaryValue* dict = NULL;
  if (!root->GetAsDictionary(&dict) || !dict) {
    return;
  }

  const base::Value* roots;
  if (!dict->Get("profile", &roots)) {
    return;
  }

  const base::DictionaryValue* roots_d_value =
      static_cast<const base::DictionaryValue*>(roots);

  const base::Value* vInfoCache;
  const base::DictionaryValue* vDictValue;
  if (roots_d_value->Get("info_cache", &vInfoCache)) {
    vInfoCache->GetAsDictionary(&vDictValue);
    for (base::DictionaryValue::Iterator it(*vDictValue); !it.IsAtEnd(); it.Advance()) {
      // const base::Value* child_copy = &it.value();
      std::string profile_name = it.key();

      const base::DictionaryValue* roots_sd_value =
          static_cast<const base::DictionaryValue*>(&it.value());

      importer::ChromeProfileInfo prof;
      const base::Value* namVal;
      if (roots_sd_value->Get("name", &namVal)) {
        base::string16 display_name;
        namVal->GetAsString(&display_name);
        prof.profileDisplayName = display_name;
      } else {
        prof.profileDisplayName = base::UTF8ToUTF16(profile_name);
      }

      prof.profileName = profile_name;

      cp->push_back(prof);
    }
  }
}
