#include "components/nfs_sync/account_info.h"

namespace nfs_sync {

AccountInfo::AccountInfo()
    : id(), name(), email() {}
AccountInfo::AccountInfo(const AccountInfo& other) = default;
AccountInfo::~AccountInfo() {}

bool AccountInfo::IsValid() const {
  return !id.empty();
}

}  // namespace nfs_sync
