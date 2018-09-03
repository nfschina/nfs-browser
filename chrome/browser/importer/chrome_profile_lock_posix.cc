#include "chrome/browser/importer/chrome_profile_lock.h"

#include "base/files/file_util.h"

void ChromeProfileLock::Init() {
}

void ChromeProfileLock::Lock() {

}

void ChromeProfileLock::Unlock() {

}

bool ChromeProfileLock::HasAcquired() {
  return !base::IsLink(lock_file_);
}

bool ChromeProfileLock::LockWithFcntl() {
  return false;
}
