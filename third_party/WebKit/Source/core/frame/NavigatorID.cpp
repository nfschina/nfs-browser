/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/frame/NavigatorID.h"
#include "base/sys_info.h"
#include "base/md5.h"
#include <string>
#include <algorithm>

#if !OS(MACOSX) && !OS(WIN)
#include "wtf/ThreadSpecific.h"
#include "wtf/Threading.h"
#include <sys/utsname.h>
#endif

namespace blink {

String NavigatorID::appCodeName() {
  return "Mozilla";
}

String NavigatorID::appName() {
  if (url().contains("https://pbank.zzbank.cn")) {
    return "Microsoft Internet Explorer";
  }
  return "Netscape";
}

String NavigatorID::appVersion() {
  // Version is everything in the user agent string past the "Mozilla/" prefix.
  const String& agent = userAgent();
  return agent.substring(agent.find('/') + 1);
}

String NavigatorID::platform() {
if (url().contains("ibsbjstar.ccb.com.cn") ||
    url().contains("perbank.abchina.com")  ||
    url().contains("bank.pingan.com.cn") ||
    url().contains("hxb.com.cn") ||
    url().contains("https://ebank.jsbchina.cn") ||
    url().contains("https://perbank.czbank.com") ||
    url().contains("https://my.hfbank.com.cn") ||
    url().contains("https://ebank.njcb.com.cn") ||
    url().contains("https://newperson.shengjingbank.com.cn") ||
    url().contains("https://person.shengjingbank.com.cn") ||
    url().contains("https://www.bankoftianjin.com") ||
    url().contains("https://ebank.cbhb.com.cn") ||
    url().contains("https://ebank-public.hzbank.com.cn") ||
    url().contains("https://ebank.grcbank.com") ||
    url().contains("https://ibank.hrbb.com.cn") ||
    url().contains("https://pibs.zyebank.com") ||
    url().contains("https://ebank.cdrcbank.com") ||
    url().contains("https://ebank.drcbank.com") ||
    url().contains("https://ibank.bankofdl.com") ||
    url().contains("https://ebank.bocd.com.cn") ||
    url().contains("https://www.bankgy.cn") ||
    url().contains("https://pbank.zzbank.cn") ||
    url().contains("https://netbank.jlbank.com.cn") ||
    url().contains("https://ebank.cqcbank.com") ||
    url().contains("https://pbank.klb.cn") ||
    url().contains("https://per.xib.com.cn") ||
    url().contains("https://ebanks.bankofshanghai.com") ||
    url().contains("https://ebank.trcbank.com.cn")) {
  return "Win32";
}

#if OS(MACOSX)
  // Match Safari and Mozilla on Mac x86.
  return "MacIntel";
#elif OS(WIN)
  // Match Safari and Mozilla on Windows.
  return "Win32";
#else  // Unix-like systems
  struct utsname osname;
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<String>, platformName,
                                  new ThreadSpecific<String>());
  if (platformName->isNull()) {
    *platformName = String(uname(&osname) >= 0
                               ? String(osname.sysname) + String(" ") +
                                     String(osname.machine)
                               : emptyString());
  }
  return *platformName;
#endif
}

String NavigatorID::product() {
  return "Gecko";
}

String NavigatorID::token() {
  //std::string mac_address = base::SysInfo::GetMacAddresNfs();
  //std::transform(mac_address.begin(), mac_address.end(),mac_address.begin(), ::toupper);
  //std::string mac_address_md5 = base::MD5String(mac_address); 
  std::string mac_address_md5 = base::SysInfo::GetMacAddresNfs();
  return String(mac_address_md5.c_str(), mac_address_md5.length());
}
}  // namespace blink
