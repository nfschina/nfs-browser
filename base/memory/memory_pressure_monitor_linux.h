// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_MEMORY_PRESSURE_MONITOR_LINUX_H_
#define BASE_MEMORY_MEMORY_PRESSURE_MONITOR_LINUX_H_

#include "base/base_export.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"

namespace base {
namespace nfs {

////////////////////////////////////////////////////////////////////////////////
// MemoryPressureMonitor
//
// A class to handle the observation of our free memory. It notifies the
// MemoryPressureListener of memory fill level changes, so that it can take
// action to reduce memory resources accordingly.
//
class BASE_EXPORT MemoryPressureMonitor : public base::MemoryPressureMonitor {
 public:

  MemoryPressureMonitor();
  ~MemoryPressureMonitor() override;

  // Get the current memory pressure level.
  MemoryPressureListener::MemoryPressureLevel GetCurrentPressureLevel() const
      override;

  void SetDispatchCallback(const DispatchCallback& callback) override;

  // Returns a type-casted version of the current memory pressure monitor. A
  // simple wrapper to base::MemoryPressureMonitor::Get.
  static MemoryPressureMonitor* Get();

  void CooldownMemPressureTimer() override;

 private:
  // Starts observing the memory fill level.
  // Calls to StartObserving should always be matched with calls to
  // StopObserving.
  void StartObserving();

  // Stop observing the memory fill level.
  // May be safely called if StartObserving has not been called.
  void StopObserving();

  // The function which gets periodically called to check any changes in the
  // memory pressure. It will report pressure changes as well as continuous
  // critical pressure levels.
  void CheckMemoryPressure();

  void ResetPressureCheckTimer(int intervalMs);

  bool cooldown_;

  // The old memory pressure.
  base::MemoryPressureListener::MemoryPressureLevel
      old_pressure_level_;

  // The current memory pressure.
  base::MemoryPressureListener::MemoryPressureLevel
      current_memory_pressure_level_;

  // A periodic timer to check for resource pressure changes.
  base::RepeatingTimer timer_;

  base::WeakPtrFactory<MemoryPressureMonitor> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MemoryPressureMonitor);
};

}  // namespace nfs
}  // namespace base

#endif  // BASE_MEMORY_MEMORY_PRESSURE_MONITOR_LINUX_H_
