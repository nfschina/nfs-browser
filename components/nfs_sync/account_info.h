#ifndef COMPONENTS_NFS_SYNC_ACCOUNT_INFO_H_
#define COMPONENTS_NFS_SYNC_ACCOUNT_INFO_H_

#include <string>

namespace nfs_sync {

// Information about a specific account.
struct AccountInfo {
  AccountInfo();
  AccountInfo(const AccountInfo& other);
  ~AccountInfo();

  std::string id;
  std::string name;
  std::string email;

  bool IsValid() const;
};

}  // namesapce nfs_sync

#endif  // COMPONENTS_NFS_SYNC_ACCOUNT_INFO_H_
