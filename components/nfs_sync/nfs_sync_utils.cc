#include "components/nfs_sync/nfs_sync_utils.h"

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "crypto/encryptor.h"
#include "crypto/symmetric_key.h"

namespace nfs_sync {

const char iv_Bookmark[] = "the iv: 32 bytes";
const char KeyBookmark[] = "bookmark_aes_cbc_passwords";
const char KeySaltBookmark[] = "bookmark_aes_cbc_salt";

std::unique_ptr<base::DictionaryValue>
JSONStringToDictionary(const std::string& json) {
  std::unique_ptr<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root.get()) {
    return nullptr;
  }

  return base::DictionaryValue::From(std::move(root));
}

bool DictionaryToString(base::DictionaryValue* dict, std::string* output) {
  return base::JSONWriter::Write(*dict, output);
}

bool EncryptAndBase64(const std::string& in, std::string* out) {
  std::unique_ptr<crypto::SymmetricKey>key_Bookmark(
      crypto::SymmetricKey::DeriveKeyFromPassword(crypto::SymmetricKey::AES,
                                                  KeyBookmark,
                                                  KeySaltBookmark,
                                                  1024, 256));

  crypto::Encryptor encryptor;
  encryptor.Init(key_Bookmark.get(),
                 crypto::Encryptor::CBC,
                 iv_Bookmark);

  std::string encrypted;
  if (!encryptor.Encrypt(in, &encrypted))
    return false;

  std::string base64_out;
  base::Base64Encode(encrypted, &base64_out);

  *out = base64_out;
  return true;
}

bool Base64DecodeAndDecrypt(const std::string& in, std::string* out) {
  std::unique_ptr<crypto::SymmetricKey>key_Bookmark(
      crypto::SymmetricKey::DeriveKeyFromPassword(crypto::SymmetricKey::AES,
                                                  KeyBookmark,
                                                  KeySaltBookmark,
                                                  1024, 256));

  std::string base64_decode;
  if (!base::Base64Decode(in, &base64_decode)) {
    return false;
  }

  crypto::Encryptor encryptor;
  encryptor.Init(key_Bookmark.get(),
                 crypto::Encryptor::CBC,
                 iv_Bookmark);

  std::string decrypted;
  if (!encryptor.Decrypt(base64_decode, &decrypted)) {
    return false;
  }

  *out = decrypted;

  return true;
}

}
