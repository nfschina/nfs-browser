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

#include "extensions/browser/pre_install/preinstall_crx_info.h"

#include "base/macros.h"
#include "build/build_config.h"

namespace extensions {

  static const std::string kAdblockId = "ehlpchdbinckoglkcpaabnfphpkjjfbk";
 // static const std::string kNightModeId = "ijkfjmnmjhjajfdefaofogchjokofjbk";   // abandon
  static const std::string kReadModeId = "anibhbkbildpjkceocooefadelkpmhkj";
  static const std::string kScreenShotId = "ppelcbgfdhcllcmkpajodenedkpbjimh";
  static const std::string kSmoothScrollId = "pbffjimnicgjgooddacophccjkepifgk";
  static const std::string kWechatId = "ibcngnihipkacaaeclcegicibajenpmi";
  static const std::string kDefaultThemeId = "amdkaoaokedfckpadjdhiggljidmgojp";
  static const std::string kActiveXId = "pmbohfpjkcmginfmpkgoiojhcejfnaha";

  // invisible extensions ( not to show up in the browser action bar)
  static const std::string kCDosPopupId = "pogijhnlcfmcppgimcaccdkmbedjkmhi";
  static const std::string kMaliciousCheckId = "kmgccjecejahmmoenlplacladmppnipc";
  static const std::string kICBCid = "aafdkgehklmaoignjmgfedeggcfbecmo";
  static const std::string kMouseGestureId = "dpidckcehhmlmdliiappkbgfobkmcekn";  

  static const std::string extensions[][2] = {
    {kSmoothScrollId, "SmoothScroll.crx"},
    {kCDosPopupId, "CDosPopup Video" },  // invisible
    {kActiveXId, "ActiveX.crx"},
#if defined(OS_WIN)
    {kICBCid, "ICBC.crx"},
#endif
  };

  // 这个版本号很重要，以后上面extension需求变动时，这个版本号加一，也会让pre install生效
  static const int version = 65;

  void GetPreInstallExtensionsInfo(PreInstallExtensionsVector& vector) {
    vector.clear();

    for (size_t i = 0; i < arraysize(extensions); ++i)  {
      PreInstallExtensionInfo info;
      info.id = extensions[i][0];
      info.name = extensions[i][1];

      vector.push_back(info);
    }
  }

  int GetVersion() {
    return version;
  }

  const std::string GetAdBlockID() {
    return kAdblockId;
  }

  const std::string GetScreenShotID() {
    return kScreenShotId;
  }

  const std::string GetCdosPopupID() {
    return kCDosPopupId;
  }

  const std::string GetWechatID() {
    return kWechatId;
  }

  const std::string GetMouseGestureID() {
    return kMouseGestureId;
  }
}
