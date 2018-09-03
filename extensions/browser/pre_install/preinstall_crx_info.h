/**
 * Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, CAS
 *
 * This software is published by the license of CPU-OS Licence, you can use and
 * distribute this software under this License. See CPU-OS License for more detail.
 *
 * You should have received a copy of CPU-OS License. If not, please contact us
 * by email <support_os@cpu-os.ac.cn>
 *
**/

#ifndef EXTENSIONS_BROWSER_PREINSTALL_PREINTALL_CRX_INFO_H_
#define EXTENSIONS_BROWSER_PREINSTALL_PREINTALL_CRX_INFO_H_
// 有新增的crx 要在该文件里面更新

#include <vector>
#include <string>

namespace extensions {
  struct PreInstallExtensionInfo {
    std::string id;
    std::string name;
  };

  typedef std::vector<PreInstallExtensionInfo> PreInstallExtensionsVector;
  typedef PreInstallExtensionsVector::iterator PreInstallExtensionsVectorIterator;

  void GetPreInstallExtensionsInfo(PreInstallExtensionsVector& vector);

  int GetVersion();

  const std::string GetAdBlockID();

  const std::string GetScreenShotID();

  const std::string GetCdosPopupID();

  const std::string GetWechatID();

  const std::string GetMouseGestureID();

}// endof extensions

#endif  // EXTENSIONS_BROWSER_PREINSTALL_PREINTALL_CRX_INFO_H_
