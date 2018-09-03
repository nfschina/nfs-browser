#include "chrome/browser/importer/chrome_profile_lock.h"

#include "base/threading/thread_restrictions.h"

#if defined(OS_POSIX)
	const base::FilePath::CharType* ChromeProfileLock::kLockFileName =
			FILE_PATH_LITERAL("SingletonLock");
#else
	const base::FilePath::CharType* ChromeProfileLock::kLockFileName =
			FILE_PATH_LITERAL("lockfile");
#endif

ChromeProfileLock::ChromeProfileLock(const base::FilePath& path) {
  base::ThreadRestrictions::SetIOAllowed(true);
  Init();
  lock_file_ = path.Append(kLockFileName);
  // LOG(ERROR) << lock_file_.value();
  Lock();
}

ChromeProfileLock::~ChromeProfileLock() {
  // Because this destructor happens in first run on the profile import thread,
  // with no UI to jank, it's ok to allow deletion of the lock here.
  // base::ThreadRestrictions::SetIOAllowed(false);
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  Unlock();
}
