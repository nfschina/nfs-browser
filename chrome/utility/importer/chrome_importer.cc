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

#include "chrome/utility/importer/chrome_importer.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/importer/chrome_importer_utils.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_codec.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

}

using namespace bookmarks;

ChromeImporter::ChromeImporter() {

}

ChromeImporter::~ChromeImporter() {

}

void ChromeImporter::LoadFile(base::FilePath& file) {
  if (!base::PathExists(file))
    return;

  std::string input;
  ReadFileToString(file, &input);

  base::JSONReader reader;
  std::unique_ptr<base::Value> root(reader.ReadToValue(input));

  base::DictionaryValue* dict = NULL;

  std::string* result = NULL;

  if (!root->GetAsDictionary(&dict)) {
    dict->GetString("checksum", result);
  }

  const base::Value* roots;
  dict->Get("roots", &roots);

  const base::DictionaryValue* roots_d_value =
      static_cast<const base::DictionaryValue*>(roots);
  const base::Value* root_folder_value;
  const base::Value* other_folder_value = nullptr;
  const base::Value* custom_roots;

  if (roots_d_value->Get("bookmark_bar", &root_folder_value) &&
      root_folder_value->IsType(base::Value::TYPE_DICTIONARY)) {
    DecodeNode(*static_cast<const base::DictionaryValue*>(root_folder_value));
  }
  if (roots_d_value->Get("other", &other_folder_value) &&
      other_folder_value->IsType(base::Value::TYPE_DICTIONARY)) {
    DecodeNode(*static_cast<const base::DictionaryValue*>(other_folder_value));
  }
  // Opera 20+ uses a custom root.
  if (roots_d_value->Get("custom_root", &custom_roots)) {
    if (custom_roots && custom_roots->IsType(base::Value::TYPE_DICTIONARY)) {
      roots_d_value = static_cast<const base::DictionaryValue*>(custom_roots);
      if (roots_d_value->Get("unsorted", &root_folder_value) &&
          root_folder_value->IsType(base::Value::TYPE_DICTIONARY)) {
        DecodeNode(
            *static_cast<const base::DictionaryValue*>(root_folder_value));
      }
      if (roots_d_value->Get("speedDial", &root_folder_value) &&
          root_folder_value->IsType(base::Value::TYPE_DICTIONARY)) {
        DecodeNode(
            *static_cast<const base::DictionaryValue*>(root_folder_value));
      }
      if (roots_d_value->Get("trash", &root_folder_value) &&
          root_folder_value->IsType(base::Value::TYPE_DICTIONARY)) {
        DecodeNode(
            *static_cast<const base::DictionaryValue*>(root_folder_value));
      }
      if (roots_d_value->Get("userRoot", &root_folder_value) &&
          root_folder_value->IsType(base::Value::TYPE_DICTIONARY)) {
        DecodeNode(
            *static_cast<const base::DictionaryValue*>(root_folder_value));
      }
    }
  }
}

/*
void ChromeImporter::ReadProfiles(std::vector<importer::ChromeProfileInfo>* cp,
                                  base::FilePath& profile_dir) {
  base::FilePath profile_filename = profile_dir.AppendASCII("Local State");
  if (!base::PathExists(profile_filename))
    return ;

  std::string input;
  base::ReadFileToString(profile_filename, &input);

  base::JSONReader reader;
  std::unique_ptr<base::Value> root(reader.ReadToValue(input));

  base::DictionaryValue* dict = NULL;
  if (!root->GetAsDictionary(&dict))
    return ;

  const base::Value* roots;
  if (!dict->Get("profile", &roots))
    return ;

  const base::DictionaryValue* roots_d_value =
      static_cast<const base::DictionaryValue*>(roots);

  const base::Value* vInfoCache;
  const base::DictionaryValue* vDictValue;
  if (roots_d_value->Get("info_cache", &vInfoCache)) {
    vInfoCache->GetAsDictionary(&vDictValue);
    for (base::DictionaryValue::Iterator it(*vDictValue); !it.IsAtEnd(); it.Advance()) {
      const base::Value* child_copy = &it.value();
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

void ChromeImporter::DetectChromeProfiles(const std::string& locale,
                                          std::vector<importer::SourceProfile>* profiles) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);
  base::FilePath profile_dir = GetChromeProfileDir();
  if (profile_dir.empty())
    return;

  importer::SourceProfile chrome;
  std::vector<importer::ChromeProfileInfo> prof;

  ReadProfiles(&prof, profile_dir);

  // chrome.importer_name = base::ASCIIToUTF16("Google Chrome");
  chrome.importer_name = l10n_util::GetStringUTF16(IDS_IMPORT_FROM_GOOGLE_CHROME);
  chrome.importer_type = importer::TYPE_CHROME;
  chrome.source_path = profile_dir;
  chrome.services_supported = importer::FAVORITES |
                              importer::HISTORY   |
                              importer::PASSWORDS;
  chrome.locale = locale;
  chrome.user_profile_names = prof;

  profiles->push_back(chrome);
}
*/

void ChromeImporter::HandleEntry(const std::string& category,
                                 const base::DictionaryValue& entries) {
  if (base::LowerCaseEqualsASCII(category, "folder")) {
    base::string16 foldername;
    AddBookmark(current_folder_, entries, true, &foldername);
    current_folder_.push_back(foldername);
  } else if (base::LowerCaseEqualsASCII(category, "url")) {
    AddBookmark(current_folder_, entries, false);
  } else if (category == "-") {
    current_folder_.pop_back();
  }
}

void ChromeImporter::AddBookmark(
    std::vector<base::string16>& current_folder,
    const base::DictionaryValue& entries,
    bool is_folder,
    base::string16* item_name) {
  std::string temp;
  base::string16 name;
  base::string16 url;
  base::string16 nickname;
  base::string16 description;
  base::string16 on_personal_bar_s;
  base::string16 in_panel_s;
  bool on_personal_bar = false;
  // bool in_panel = false;
  // int personal_bar_pos = 0;
  // int panel_pos = 0;

  double created_time = 0;
  double visited_time = 0;

  if (!is_folder && !entries.GetString("url", &url))
    url = base::string16();

  if (!entries.GetString("name", &name))
    name = url;

  if (item_name)
   *item_name = name;

  if (!entries.GetString("short name", &nickname))
    nickname.clear();
  if (!entries.GetString("description", &description))
    description.clear();

  if (!entries.GetString("on personalbar", &on_personal_bar_s))
    on_personal_bar = (base::LowerCaseEqualsASCII(on_personal_bar_s, "yes"));

  if (!entries.GetString("created", &temp) ||
      !base::StringToDouble(temp, &created_time)) {
    created_time = 0;
  }

  if (!entries.GetString("visited", &temp) ||
      !base::StringToDouble(temp, &visited_time)) {
    visited_time = 0;
  }

  ImportedBookmarkEntry entry;
  entry.in_toolbar = false; //on_personal_bar;
  entry.is_folder = is_folder;
  entry.title = name;
  entry.nickname = nickname;
  entry.description = description;
  entry.path = current_folder;
  entry.url = GURL(url);
  entry.creation_time = base::Time::FromTimeT(created_time);
  entry.visited_time = base::Time::FromTimeT(visited_time);

  bookmarks_.push_back(entry);
}

void ChromeImporter::StartImport(const importer::SourceProfile& source_profile,
                                 uint16_t items,
                                 ImporterBridge* bridge) {
  DCHECK(bridge);
  if (!bridge) {
    return;
  }

  bridge_ = bridge;
  source_path_ = source_profile.source_path;

  std::string name = source_profile.selected_profile_name;
  if (name.empty())
    profile_path_ = source_profile.source_path.AppendASCII("Default");
  else
    profile_path_ = source_profile.source_path.AppendASCII(name);

#if defined(OS_POSIX)
  locale_ = source_profile.locale;
#endif

  bridge_->NotifyStarted();

  if ((items & importer::FAVORITES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::FAVORITES);
    ImportBookmarks();
    bridge_->NotifyItemEnded(importer::FAVORITES);
  }

  bridge_->NotifyEnded();
}

void ChromeImporter::ImportBookmarks() {
  bookmarks_path_ = profile_path_.AppendASCII("Bookmarks");
  if (!base::PathExists(bookmarks_path_)) {
    // bridge_->NotifyEnded();
    return ;
  }

  LoadFile(bookmarks_path_);

  if (!Bookmarks().empty() && !cancelled()) {
    const base::string16& first_folder_name =
        bridge_->GetLocalizedString(IDS_BOOKMARK_GROUP_FROM_CHROME);
    LOG(ERROR) << first_folder_name;

    bridge_->AddBookmarks(Bookmarks(), first_folder_name);

    //  reader.ClearBookmarks();
  }
}

bool ChromeImporter::DecodeNode(const base::DictionaryValue& value) {
  std::string id_string;
  // int64_t id = 0;

  base::string16 title;
  value.GetString(BookmarkCodec::kNameKey, &title);

  std::string date_added_string;

  std::string type_string;
  if (!value.GetString(BookmarkCodec::kTypeKey, &type_string))
    return false;

  if (type_string != BookmarkCodec::kTypeURL && type_string != BookmarkCodec::kTypeFolder)
    return false;  // Unknown type.

  if (type_string == BookmarkCodec::kTypeURL) {
    std::string url_string;
    if (!value.GetString(BookmarkCodec::kURLKey, &url_string))
      return false;

    GURL url = GURL(url_string);

    HandleEntry("url", value);

  } else {
    const base::Value* child_values;
    if (!value.Get(BookmarkCodec::kChildrenKey, &child_values))
      return false;

    if (child_values->GetType() != base::Value::TYPE_LIST)
      return false;

    const base::ListValue* list_values =
        static_cast<const base::ListValue*>(child_values);

    // Skip empty folders.
    if (!list_values->empty()) {
      HandleEntry("folder", value);
      DecodeChildren(*list_values);
    }
  }
  return true;
}

bool ChromeImporter::DecodeChildren(
  const base::ListValue& child_value_list) {

  for (size_t i = 0; i < child_value_list.GetSize(); ++i) {
    const base::Value* child_value;
    if (!child_value_list.Get(i, &child_value))
      return false;

    if (child_value->GetType() != base::Value::TYPE_DICTIONARY)
      return false;

    DecodeNode(*static_cast<const base::DictionaryValue*>(child_value));
  }

  base::DictionaryValue* dict = NULL;

  HandleEntry("-", *dict);
  return true;
}
