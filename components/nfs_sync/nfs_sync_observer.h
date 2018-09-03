#ifndef COMPONENTS_NFS_SYNC_NFS_SYNC_OBSERVER_H_
#define COMPONENTS_NFS_SYNC_NFS_SYNC_OBSERVER_H_

namespace nfs_sync {

// Observer for the NfsSync.
class NfsSyncObserver {
public:
  // Triggered when login successfully.
  virtual void OnLoginSuccess(const std::string& name,
                              const std::string& email,
                              const std::string& id) = 0;

  // Triggered when logout successfully.
  virtual void OnLogoutSuccess() = 0;

protected:
  virtual ~NfsSyncObserver() {}
};  // class NfsSyncObserver

}  // namespace nfs_sync

#endif  // COMPONENTS_NFS_SYNC_NFS_SYNC_OBSERVER_H_
