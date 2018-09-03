// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/sys_info.h"

#include <windows.h>
#include <stddef.h>
#include <stdint.h>

/****************added by wangpp******************/
#include <winsock2.h>
#include <Iphlpapi.h>
#include <stdio.h>
/*************************************************/

#include <limits>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/windows_version.h"
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")


static char s_macAddress[2048];

namespace {

int64_t AmountOfMemory(DWORDLONG MEMORYSTATUSEX::*memory_field) {
  MEMORYSTATUSEX memory_info;
  memory_info.dwLength = sizeof(memory_info);
  if (!GlobalMemoryStatusEx(&memory_info)) {
    NOTREACHED();
    return 0;
  }

  int64_t rv = static_cast<int64_t>(memory_info.*memory_field);
  return rv < 0 ? std::numeric_limits<int64_t>::max() : rv;
}

bool GetDiskSpaceInfo(const base::FilePath& path,
                      int64_t* available_bytes,
                      int64_t* total_bytes) {
  ULARGE_INTEGER available;
  ULARGE_INTEGER total;
  ULARGE_INTEGER free;
  if (!GetDiskFreeSpaceExW(path.value().c_str(), &available, &total, &free))
    return false;

  if (available_bytes) {
    *available_bytes = static_cast<int64_t>(available.QuadPart);
    if (*available_bytes < 0)
      *available_bytes = std::numeric_limits<int64_t>::max();
  }
  if (total_bytes) {
    *total_bytes = static_cast<int64_t>(total.QuadPart);
    if (*total_bytes < 0)
      *total_bytes = std::numeric_limits<int64_t>::max();
  }
  return true;
}

}  // namespace

namespace base {

// static
int SysInfo::NumberOfProcessors() {
  return win::OSInfo::GetInstance()->processors();
}

// static
int64_t SysInfo::AmountOfPhysicalMemory() {
  return AmountOfMemory(&MEMORYSTATUSEX::ullTotalPhys);
}

// static
int64_t SysInfo::AmountOfAvailablePhysicalMemory() {
  return AmountOfMemory(&MEMORYSTATUSEX::ullAvailPhys);
}

// static
int64_t SysInfo::AmountOfVirtualMemory() {
  return AmountOfMemory(&MEMORYSTATUSEX::ullTotalVirtual);
}

// static
int64_t SysInfo::AmountOfFreeDiskSpace(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();

  int64_t available;
  if (!GetDiskSpaceInfo(path, &available, nullptr))
    return -1;
  return available;
}

// static
int64_t SysInfo::AmountOfTotalDiskSpace(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();

  int64_t total;
  if (!GetDiskSpaceInfo(path, nullptr, &total))
    return -1;
  return total;
}

std::string SysInfo::OperatingSystemName() {
  std::string version = OperatingSystemVersion();
  if (version.find("5.1") == 0) {
    return "Windows XP";
  } else if (version.find("6.0") == 0) {
    return "Windows Vista";
  } else if (version.find("6.1") == 0) {
    return "Windows 7";
  } else if (version.find("6.2") == 0) {
    return "Windows 8";
  } else if (version.find("6.3") == 0) {
    return "Windows 8.1";
  } else if (version.find("10.0") == 0) {
    return "Windows 10";
  } else {
    return "Windows NT";
  }
}

// static
std::string SysInfo::GetSystemName() {
  return "Windows";
}

// static
std::string SysInfo::OperatingSystemVersion() {
  win::OSInfo* os_info = win::OSInfo::GetInstance();
  win::OSInfo::VersionNumber version_number = os_info->version_number();
  std::string version(StringPrintf("%d.%d.%d", version_number.major,
                                   version_number.minor,
                                   version_number.build));
  win::OSInfo::ServicePack service_pack = os_info->service_pack();
  if (service_pack.major != 0) {
    version += StringPrintf(" SP%d", service_pack.major);
    if (service_pack.minor != 0)
      version += StringPrintf(".%d", service_pack.minor);
  }
  return version;
}

// TODO: Implement OperatingSystemVersionComplete, which would include
// patchlevel/service pack number.
// See chrome/browser/feedback/feedback_util.h, FeedbackUtil::SetOSVersion.

// static
std::string SysInfo::OperatingSystemArchitecture() {
  win::OSInfo::WindowsArchitecture arch =
      win::OSInfo::GetInstance()->architecture();
  switch (arch) {
    case win::OSInfo::X86_ARCHITECTURE:
      return "x86";
    case win::OSInfo::X64_ARCHITECTURE:
      return "x86_64";
    case win::OSInfo::IA64_ARCHITECTURE:
      return "ia64";
    default:
      return "";
  }
}

// static
std::string SysInfo::CPUModelName() {
  return win::OSInfo::GetInstance()->processor_model_name();
}

// static
size_t SysInfo::VMAllocationGranularity() {
  return win::OSInfo::GetInstance()->allocation_granularity();
}

// static
void SysInfo::OperatingSystemVersionNumbers(int32_t* major_version,
                                            int32_t* minor_version,
                                            int32_t* bugfix_version) {
  win::OSInfo* os_info = win::OSInfo::GetInstance();
  *major_version = os_info->version_number().major;
  *minor_version = os_info->version_number().minor;
  *bugfix_version = 0;
}

// static
std::string SysInfo::GetMacAddress() {
  ULONG ulSize = 0;
  PIP_ADAPTER_INFO pInfo = NULL;
  // 第一次调用获取缓冲的大小
  GetAdaptersInfo(pInfo, &ulSize);
  pInfo = (PIP_ADAPTER_INFO)malloc(ulSize);
  if (GetAdaptersInfo(pInfo, &ulSize) != NO_ERROR) {
    return std::string();
  }

  std::vector<std::string> mac_list;
  mac_list.clear();

  while(pInfo) {
    std::string mac("");
    for (int i = 0; i < (int)pInfo->AddressLength; i++) {
      mac += StringPrintf("%02x", (int)pInfo->Address[i]);
      if (i < (int)pInfo->AddressLength - 1)
        mac += ":";
    }

    mac_list.push_back(mac);
    pInfo = pInfo->Next;
  }

  if (mac_list.empty())
    return std::string();

  std::string mac_address = mac_list[0];
  std::transform(mac_address.begin(), mac_address.end(), mac_address.begin(), ::toupper);
  return mac_address;
}

// static
std::string SysInfo::GetMacAddressMd5() {
  ULONG ulSize = 0;
  PIP_ADAPTER_INFO pInfo = NULL;
  // 第一次调用获取缓冲的大小
  GetAdaptersInfo(pInfo, &ulSize);
  pInfo = (PIP_ADAPTER_INFO)malloc(ulSize);
  if (GetAdaptersInfo(pInfo, &ulSize) != NO_ERROR) {
    return std::string();
  }

  std::vector<std::string> mac_list;
  mac_list.clear();

  while(pInfo) {
    std::string mac("");
    for (int i = 0; i < (int)pInfo->AddressLength; i++) {
      mac += StringPrintf("%02x", (int)pInfo->Address[i]);
      if (i < (int)pInfo->AddressLength - 1)
        mac += ":";
    }

    mac_list.push_back(mac);
    pInfo = pInfo->Next;
  }

  if (mac_list.empty())
    return std::string();

  std::string mac_address;
  std::string mac_address_md5;
  for(unsigned int i = 0; i < mac_list.size(); i++) {
    mac_address = mac_list[i];
    std::transform(mac_address.begin(), mac_address.end(),mac_address.begin(), ::toupper);
    mac_address_md5.append(base::MD5String(mac_address));
    mac_address_md5.append(";");
  }
  SetMacAddresNfs(mac_address_md5);
  return mac_address_md5;
}

// static
std::string SysInfo::GetMacAddresNfs() {
  return std::string(s_macAddress);
}

// static
void SysInfo::SetMacAddresNfs(std::string mac_address) {
  strncpy(s_macAddress, mac_address.c_str(), mac_address.size());
}

}  // namespace base
