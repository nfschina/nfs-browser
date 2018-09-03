// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/sys_info.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <limits>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/sys_info_internal.h"
#include "build/build_config.h"

static char s_macAddress[2048];

namespace {

int64_t AmountOfMemory(int pages_name) {
  long pages = sysconf(pages_name);
  long page_size = sysconf(_SC_PAGESIZE);
  if (pages == -1 || page_size == -1) {
    NOTREACHED();
    return 0;
  }
  return static_cast<int64_t>(pages) * page_size;
}

int64_t AmountOfPhysicalMemory() {
  return AmountOfMemory(_SC_PHYS_PAGES);
}

std::string GetReleaseInfoByKey(const std::string& key) {
  const bool io_allowed = base::ThreadRestrictions::SetIOAllowed(true);

  FILE* in;

  if (key == "Distributor ID") {
    in = popen("lsb_release -i", "r");
  } else if (key == "Release") {
    in = popen("lsb_release -r", "r");
  } else {
    return std::string();
  }

  if (!in) {
    return std::string();
  }

  char* line = NULL;
  size_t len = 0;
  std::string value = std::string();
  while(getline(&line, &len, in) != -1) {
    if (line) {
      size_t strLen = strlen(line);
      line[strLen - 1] = '\0';

      std::vector<std::string> vec =
          base::SplitString(std::string(line), ":",
                            base::TRIM_WHITESPACE,
                            base::SPLIT_WANT_NONEMPTY);
      if (!vec.empty() && vec.size() > 1 && key == vec[0]) {
        value = vec[1];
        break;
      }
    }
  }

  free(line);
  pclose(in);

  base::ThreadRestrictions::SetIOAllowed(io_allowed);

  return value;
}

base::LazyInstance<
    base::internal::LazySysInfoValue<int64_t, AmountOfPhysicalMemory>>::Leaky
    g_lazy_physical_memory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace base {

// static
int64_t SysInfo::AmountOfAvailablePhysicalMemory() {
  return AmountOfMemory(_SC_AVPHYS_PAGES);
}

// static
int64_t SysInfo::AmountOfPhysicalMemory() {
  return g_lazy_physical_memory.Get().value();
}

// static
std::string SysInfo::CPUModelName() {
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL)
  const char kCpuModelPrefix[] = "Hardware";
#else
  const char kCpuModelPrefix[] = "model name";
#endif
  std::string contents;
  ReadFileToString(FilePath("/proc/cpuinfo"), &contents);
  DCHECK(!contents.empty());
  if (!contents.empty()) {
    std::istringstream iss(contents);
    std::string line;
    while (std::getline(iss, line)) {
      if (line.compare(0, strlen(kCpuModelPrefix), kCpuModelPrefix) == 0) {
        size_t pos = line.find(": ");
        return line.substr(pos + 2);
      }
    }
  }
  return std::string();
}

/* added by wangpp */
// static
std::string SysInfo::GetMacAddress() {
  int sock_mac;
  char mac[18];
  struct ifreq ifr;
  struct ifaddrs* if_addrs = NULL;
  struct ifaddrs* ifa = NULL;
  // void* tmp_addr = NULL;

  sock_mac = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sock_mac)
    return std::string();

  getifaddrs(&if_addrs);
  for (ifa = if_addrs; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr)
      continue;

    if (strlen(ifa->ifa_name) == 0 || strncmp(ifa->ifa_name, "lo", 2) == 0)
      continue;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, (char*)ifa->ifa_name, sizeof(ifa->ifa_name));
    if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr) < 0)
      continue;

    memset(mac, '\0', 18);
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifr.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

    SetMacAddresNfs(mac);

    std::string mac_str(mac);
    std::transform(mac_str.begin(), mac_str.end(),mac_str.begin(), ::toupper);
    return mac_str;
  }

  return std::string();
}

// static
std::string SysInfo::GetMacAddressMd5() {
  int sock_mac;
  char mac[18];
  struct ifreq ifr;
  struct ifaddrs* if_addrs = NULL;
  struct ifaddrs* ifa = NULL;
  // void* tmp_addr = NULL;

  sock_mac = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sock_mac)
    return std::string();

  std::vector<std::string> mac_list;
  mac_list.clear();
  getifaddrs(&if_addrs);
  for (ifa = if_addrs; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr)
      continue;

    if (strlen(ifa->ifa_name) == 0 || strncmp(ifa->ifa_name, "lo", 2) == 0)
      continue;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, (char*)ifa->ifa_name, sizeof(ifa->ifa_name));
    if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr) < 0)
      continue;

    memset(mac, '\0', 18);
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifr.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

    // mac_list.push_back(mac);
    // if (mac_list.find(std::string(mac)) != mac_list.end())
    if (std::find(mac_list.begin(), mac_list.end(), std::string(mac)) != mac_list.end())
      continue;
    mac_list.push_back(mac);
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
std::string SysInfo::GetSystemInfo() {
  std::string name = GetSystemName();
  std::string version = GetSystemVersion();
  if (name.empty() || version.empty())
    return std::string();
  return name + " " + version;
}

// static
std::string SysInfo::GetSystemName() {
  return GetReleaseInfoByKey("Distributor ID");
}

// static
std::string SysInfo::GetSystemVersion() {
  return GetReleaseInfoByKey("Release");
}
/* added by wangpp */

// static
std::string SysInfo::GetMacAddresNfs() {
  return std::string(s_macAddress);
}

//static
void SysInfo::SetMacAddresNfs(std::string mac_address) {
  strncpy(s_macAddress, mac_address.c_str(), mac_address.size());
}


}  // namespace base
