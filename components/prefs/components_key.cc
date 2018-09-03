
#include "components/prefs/components_key.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/threading/thread_restrictions.h"
#include "base/version.h"
#include "base/rand_util.h"
#include "base/base64.h"
#include "crypto/encryptor.h"
#include "crypto/symmetric_key.h"


namespace  components 
{

const std::string iv_json("the iv: 16 bytes");
// const   std::string iv_Bookmark ("the iv: 32 bytes");
// const std::string   KeyBookmark = "bookmark_aes_cbc_passwords";
// const std::string   KeySaltBookmark = "bookmark_aes_cbc_salt";
const std::string   KkeyStringSalt = "json_aes_cbc_salt";
const std::string   KkeyStringKey = "json_aes_cbc_passwords";
// size_t KeyBookmarkValue = 1024;
// size_t KeySaltBookmarkValue =  256;
// size_t   KkeyStringKeyValue = 1000;;
// size_t  KkeyStringSaltValue =  256;
// const std::unique_ptr<crypto::SymmetricKey>key_Bookmark (crypto::SymmetricKey::DeriveKeyFromPassword(
		  // crypto::SymmetricKey::AES, KeyBookmark, KeySaltBookmark, 1024, 256));
const std::unique_ptr<crypto::SymmetricKey>key_json (crypto::SymmetricKey::DeriveKeyFromPassword(
		  crypto::SymmetricKey::AES, KkeyStringKey, KkeyStringSalt, 1000, 256));
const char kDisableEncrypt[] = "no-sec-pref";
// bool NeedEncrypt(){
//      return 
//         !base::CommandLine::ForCurrentProcess()->HasSwitch(
//              ::switches::kDisableSecurePreference);
// }
}  // namespace chrome
