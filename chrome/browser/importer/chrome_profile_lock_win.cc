#include "chrome/browser/importer/chrome_profile_lock.h"

#include <windows.h>


void ChromeProfileLock::Init() {
  lock_handle_ = INVALID_HANDLE_VALUE;
}

void ChromeProfileLock::Lock() {
  if (HasAcquired())
    return;
  lock_handle_ = CreateFile(lock_file_.value().c_str(),
                            GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                            FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

void ChromeProfileLock::Unlock() {
  if (!HasAcquired())
    return;
  CloseHandle(lock_handle_);
  lock_handle_ = INVALID_HANDLE_VALUE;
}

bool ChromeProfileLock::HasAcquired() {
  return (lock_handle_ != INVALID_HANDLE_VALUE);
}
