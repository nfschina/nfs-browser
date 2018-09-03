#ifndef COMPONENTS_NFS_SYNC_NFS_SYNC_UTILS_H_
#define COMPONENTS_NFS_SYNC_NFS_SYNC_UTILS_H_

#include <memory>
#include <string>

namespace base {
class DictionaryValue;
}

namespace nfs_sync {

std::unique_ptr<base::DictionaryValue>
JSONStringToDictionary(const std::string& json);

bool DictionaryToString(base::DictionaryValue* value, std::string* output);

bool EncryptAndBase64(const std::string& in, std::string* out);
bool Base64DecodeAndDecrypt(const std::string& in, std::string* out);

}

#endif  // COMPONENTS_NFS_SYNC_NFS_SYNC_UTILS_H_
