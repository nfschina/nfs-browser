// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#include "chrome/browser/ui/browser_ui_prefs.h"

#include "build/build_config.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/translate/core/common/translate_pref_names.h"
#include "content/public/common/webrtc_ip_handling_policy.h"

namespace {

uint32_t GetHomeButtonAndHomePageIsNewTabPageFlags() {
#if defined(OS_IOS) || defined(OS_ANDROID)
  return PrefRegistry::NO_REGISTRATION_FLAGS;
#else
  return user_prefs::PrefRegistrySyncable::SYNCABLE_PREF;
#endif
}

}  // namespace

namespace chrome {

void RegisterBrowserPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kOptionsWindowLastTabIndex, 0);
  registry->RegisterBooleanPref(prefs::kAllowFileSelectionDialogs, true);
  registry->RegisterIntegerPref(prefs::kShowFirstRunBubbleOption,
                             first_run::FIRST_RUN_BUBBLE_DONT_SHOW);
}

void RegisterBrowserUserPrefs(user_prefs::PrefRegistrySyncable* registry) {
    registry->RegisterDictionaryPref(
      prefs::kShortCutsTable,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
   registry->RegisterBooleanPref(
      prefs::kEnableShortCuts,
                true,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
      registry->RegisterIntegerPref(
      prefs::kDiskMaxCacheSize,
                100,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
        registry->RegisterBooleanPref(
      prefs::kDiskCahceAutoclear,
                false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
   registry->RegisterBooleanPref(
      prefs::kShowSearchBar,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
    registry->RegisterBooleanPref(
      prefs::kShowMarkedBar,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
        registry->RegisterBooleanPref(
      prefs::kShowToolbar,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
        registry->RegisterIntegerPref(
      prefs::kBrowseringMode,
                0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

    // the address bar func show
    registry->RegisterBooleanPref(
      prefs::kAddressAutofill,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
   registry->RegisterBooleanPref(
      prefs::kAddressHistory,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
      registry->RegisterBooleanPref(
      prefs::kAddressSaved,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
      registry->RegisterBooleanPref(
      prefs::kAddressClip,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
      registry->RegisterBooleanPref(
      prefs::kAddressCompatibleMode,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
//end  addressbar
  //add  for the tag page setting
  registry->RegisterIntegerPref(
      prefs::kNewTabPage,
      0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kCloseMultiTabs,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
      registry->RegisterBooleanPref(
      prefs::kCloseAutoClearBrowserData,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kDoubleClickToClose,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
    registry->RegisterBooleanPref(
      prefs::kClickToCloseNotice,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  // 0 表示关闭标签时智能切换
  // 1 表示关闭标签时切换到右侧的标签
  // 2 表示关闭标签时切换到左侧的标签
  registry->RegisterIntegerPref(
      prefs::kCloseTabSwitch,
      0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kOpenNewTabPage,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kBoldInBackground,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kPomptInNewWindow,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  // huk  1表示在最最右边打开
   registry->RegisterIntegerPref(
          prefs::kOpenDirection,
          0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

   // 标签预览功能，默认开启
   registry->RegisterIntegerPref(
          prefs::kEnableTabPreview,
          2,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  //end  for  the tab pages
    registry->RegisterIntegerPref(
      prefs::kTabMaxIndex,
                80,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

    registry->RegisterBooleanPref(
          prefs::kEnableClickPrerender,
          false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

    registry->RegisterBooleanPref(
          prefs::kEnableMouseOverPrerender,
          false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  // registry->RegisterStringPref(
  //     prefs::kHomePage,
  //     std::string("https://www.baidu.com/"),
  //     user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  // registry->RegisterBooleanPref(
  //     prefs::kHomePageIsNewTabPage,
  //     true,
  //     user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  // registry->RegisterBooleanPref(
  //     prefs::kShowHomeButton,
  //     false,
  //     user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kHomePageIsNewTabPage,
      true,
      GetHomeButtonAndHomePageIsNewTabPageFlags());
  registry->RegisterBooleanPref(prefs::kShowHomeButton, false,
                                GetHomeButtonAndHomePageIsNewTabPageFlags());

   registry->RegisterBooleanPref(
      prefs::kLockKeyword,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterIntegerPref(prefs::kModuleConflictBubbleShown, 0);
  registry->RegisterInt64Pref(prefs::kDefaultBrowserLastDeclined, 0);
  bool reset_check_default = false;
#if defined(OS_WIN)
  reset_check_default = base::win::GetVersion() >= base::win::VERSION_WIN10;
#endif
  registry->RegisterBooleanPref(prefs::kResetCheckDefaultBrowser,
                                reset_check_default);
  registry->RegisterBooleanPref(prefs::kWebAppCreateOnDesktop, true);
  registry->RegisterBooleanPref(prefs::kWebAppCreateInAppsMenu, true);
  registry->RegisterBooleanPref(prefs::kWebAppCreateInQuickLaunchBar, true);
  registry->RegisterBooleanPref(
      prefs::kEnableTranslate,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterStringPref(prefs::kCloudPrintEmail, std::string());
  registry->RegisterBooleanPref(prefs::kCloudPrintProxyEnabled, true);
  registry->RegisterBooleanPref(prefs::kCloudPrintSubmitEnabled, true);
  registry->RegisterBooleanPref(prefs::kDevToolsDisabled, false);
  registry->RegisterDictionaryPref(prefs::kBrowserWindowPlacement);
  registry->RegisterDictionaryPref(prefs::kBrowserWindowPlacementPopup);
  registry->RegisterDictionaryPref(prefs::kAppWindowPlacement);
  registry->RegisterBooleanPref(prefs::kImportAutofillFormData, true);
  registry->RegisterBooleanPref(prefs::kImportBookmarks, true);
  registry->RegisterBooleanPref(prefs::kImportHistory, true);
  registry->RegisterBooleanPref(prefs::kImportHomepage, true);
  registry->RegisterBooleanPref(prefs::kImportSavedPasswords, true);
  registry->RegisterBooleanPref(prefs::kImportSearchEngine, true);
  registry->RegisterBooleanPref(
      prefs::kEnableDoNotTrack,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kEnablePrivacyProtocal,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
#if defined(ENABLE_WEBRTC)
  // TODO(guoweis): Remove next 2 options at M50.
  registry->RegisterBooleanPref(prefs::kWebRTCMultipleRoutesEnabled, true);
  registry->RegisterBooleanPref(prefs::kWebRTCNonProxiedUdpEnabled, true);
  registry->RegisterStringPref(prefs::kWebRTCIPHandlingPolicy,
                               content::kWebRTCIPHandlingDefault);
  registry->RegisterStringPref(prefs::kWebRTCUDPPortRange, std::string());
#endif

  // Dictionaries to keep track of default tasks in the file browser.
  registry->RegisterDictionaryPref(
      prefs::kDefaultTasksByMimeType,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      prefs::kDefaultTasksBySuffix,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  // We need to register the type of these preferences in order to query
  // them even though they're only typically controlled via policy.
  registry->RegisterBooleanPref(prefs::kPluginsAllowOutdated, true); //huk set it to true (bug #349394)
  registry->RegisterBooleanPref(prefs::kPluginsAlwaysAuthorize, false);
  registry->RegisterBooleanPref(prefs::kClearPluginLSODataEnabled, true);
  registry->RegisterBooleanPref(prefs::kHideWebStoreIcon, false);
#if defined(OS_MACOSX)
  // This really belongs in platform code, but there's no good place to
  // initialize it between the time when the AppController is created
  // (where there's no profile) and the time the controller gets another
  // crack at the start of the main event loop. By that time,
  // StartupBrowserCreator has already created the browser window, and it's too
  // late: we need the pref to be already initialized. Doing it here also saves
  // us from having to hard-code pref registration in the several unit tests
  // that use this preference.
  registry->RegisterBooleanPref(prefs::kShowUpdatePromotionInfoBar, true);
  registry->RegisterBooleanPref(
      prefs::kShowFullscreenToolbar,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kHideFullscreenToolbar,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
#else
  registry->RegisterBooleanPref(prefs::kFullscreenAllowed, true);
#endif

  registry->RegisterBooleanPref(
      prefs::kAutoUpdateToLatest,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterBooleanPref(
      prefs::kShowVideoToolbar,
      true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterBooleanPref(
      prefs::kShowCopySearchToolbar,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

}  // namespace chrome
