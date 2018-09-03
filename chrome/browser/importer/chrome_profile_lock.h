#ifndef CHROME_IMPORTER_CHROME_PROFILE_LOCK_H_
#define CHROME_IMPORTER_CHROME_PROFILE_LOCK_H_

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

class ChromeProfileLock {
 public:
   explicit ChromeProfileLock(const base::FilePath& path);
   ~ChromeProfileLock();

  // Locks and releases the profile.
  void Lock();
  void Unlock();

  // Returns true if we lock the profile successfully.
  bool HasAcquired();

 private:

  static const base::FilePath::CharType* kLockFileName;

  void Init();

  // Full path of the lock file in the profile folder.
  base::FilePath lock_file_;

  // The handle of the lock file.
#if defined(OS_WIN)
  HANDLE lock_handle_;
#elif defined(OS_POSIX)
  int lock_fd_;

  // Method that tries to put a fcntl lock on file specified by |lock_file_|.
  // Returns false if lock is already held by another process. true in all
  // other cases.
  bool LockWithFcntl();
#endif

  DISALLOW_COPY_AND_ASSIGN(ChromeProfileLock);
};

#endif	// CHROME_IMPORTER_CHROME_PROFILE_LOCK_H_
