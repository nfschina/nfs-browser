// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/memory_pressure_monitor_linux.h"

#include <fcntl.h>
#include <sys/select.h>

#include "base/process/process_metrics.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_info.h"
// #include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

namespace base {
namespace nfs {

namespace {

// The time between memory pressure checks.
const int kCommonMemoryPressureIntervalMs = 10000;
const int kCriticalMemoryPressureIntervalMs = 1000;
const int kPurgeMemThresholdPC = 85;

}  // namespace

MemoryPressureMonitor::MemoryPressureMonitor()
    : cooldown_(false),
      old_pressure_level_(
          MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE),
      current_memory_pressure_level_(
          MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE),
      weak_ptr_factory_(this) {
  StartObserving();
}

MemoryPressureMonitor::~MemoryPressureMonitor() {
  StopObserving();
}

MemoryPressureListener::MemoryPressureLevel
MemoryPressureMonitor::GetCurrentPressureLevel() const {
  return current_memory_pressure_level_;
}

// static
MemoryPressureMonitor* MemoryPressureMonitor::Get() {
  return static_cast<MemoryPressureMonitor*>(
      base::MemoryPressureMonitor::Get());
}

void MemoryPressureMonitor::StartObserving() {
  ResetPressureCheckTimer(kCommonMemoryPressureIntervalMs);
}

void MemoryPressureMonitor::StopObserving() {
  // If StartObserving failed, StopObserving will still get called.
  timer_.Stop();
}

void MemoryPressureMonitor::CheckMemoryPressure() {
  base::SystemMemoryInfoKB info;
  if (!base::GetSystemMemoryInfo(&info)) {
    VLOG(1) << "Cannot determine the free memory of the system.";
    return;
  }

  int percentage = ((info.total - info.free - info.buffers - info.cached) * 100) / info.total;
  current_memory_pressure_level_ = percentage > kPurgeMemThresholdPC  ?
         MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL :
         MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;

  // In case there is no memory pressure we do not notify.
  if (current_memory_pressure_level_ == MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE) {
    if (old_pressure_level_ == MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL) {
          ResetPressureCheckTimer(kCommonMemoryPressureIntervalMs);
    }
    if (old_pressure_level_ != current_memory_pressure_level_) {
      old_pressure_level_ = current_memory_pressure_level_;
    }
    return;
  }

  if (old_pressure_level_ == MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE &&
        current_memory_pressure_level_ == MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL) {
    ResetPressureCheckTimer(kCriticalMemoryPressureIntervalMs);
  }

  if (old_pressure_level_ == MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL
        && current_memory_pressure_level_ == MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL
        && cooldown_) {
    cooldown_ = false;
    ResetPressureCheckTimer(kCommonMemoryPressureIntervalMs);
  }

  if (old_pressure_level_ != current_memory_pressure_level_) {
    old_pressure_level_ = current_memory_pressure_level_;
  }
  
  MemoryPressureListener::NotifyMemoryPressure(current_memory_pressure_level_);
}

void MemoryPressureMonitor::SetDispatchCallback(const DispatchCallback& callback){
}

void MemoryPressureMonitor::CooldownMemPressureTimer() {
  cooldown_ = true;
}

void MemoryPressureMonitor::ResetPressureCheckTimer(int intervalMs) {
  timer_.Start(FROM_HERE,
               TimeDelta::FromMilliseconds(intervalMs),
               Bind(&MemoryPressureMonitor::CheckMemoryPressure,
                    weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace nfs
}  // namespace base
