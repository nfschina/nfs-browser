#ifndef CHROME_COMMON_CHROME_KEY_H_
#define CHROME_COMMON_CHROME_KEY_H_

#include "build/build_config.h"
#include "crypto/encryptor.h"
#include "crypto/symmetric_key.h"


namespace chrome {
// extern   const std::string iv_json;
extern const std::string iv_Bookmark;
extern const std::unique_ptr<crypto::SymmetricKey>key_Bookmark;
extern const char  kDisableEncrypt_Bookmark[];   //wdg add for the preferneces &bookmark encrypt
extern const char kStatisticID[];

// extern    const std::unique_ptr<crypto::SymmetricKey>key_json;

}  // namespace switches

#endif  // CHROME_COMMON_CHROME_KEY_H_
