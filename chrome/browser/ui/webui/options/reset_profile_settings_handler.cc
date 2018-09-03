// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#include "chrome/browser/ui/webui/options/reset_profile_settings_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/google/google_brand.h"
#include "chrome/browser/profile_resetter/brandcode_config_fetcher.h"
#include "chrome/browser/profile_resetter/brandcoded_default_settings.h"
#include "chrome/browser/profile_resetter/profile_reset_report.pb.h"
#include "chrome/browser/profile_resetter/profile_resetter.h"
#include "chrome/browser/profile_resetter/resettable_settings_snapshot.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_WIN)
#include "chrome/browser/profile_resetter/triggered_profile_resetter.h"
#include "chrome/browser/profile_resetter/triggered_profile_resetter_factory.h"
#endif  // defined(OS_WIN)
#include "chrome/common/pref_names.h"  //wdg
#include "content/public/browser/host_zoom_map.h"
#include "chrome/browser/ui/zoom/chrome_zoom_level_prefs.h"
#include "components/browsing_data/core/pref_names.h"

namespace options {

namespace {

reset_report::ChromeResetReport::ResetRequestOrigin
ResetRequestOriginFromString(const std::string& reset_request_origin) {
  static const char kOriginUserClick[] = "userclick";
  static const char kOriginTriggeredReset[] = "triggeredreset";

  if (reset_request_origin ==
      ResetProfileSettingsHandler::kCctResetSettingsHash) {
    return reset_report::ChromeResetReport::RESET_REQUEST_ORIGIN_CCT;
  }
  if (reset_request_origin == kOriginUserClick)
    return reset_report::ChromeResetReport::RESET_REQUEST_ORIGIN_USER_CLICK;
  if (reset_request_origin == kOriginTriggeredReset) {
    return reset_report::ChromeResetReport::
        RESET_REQUEST_ORIGIN_TRIGGERED_RESET;
  }
  if (!reset_request_origin.empty())
    NOTREACHED();

  return reset_report::ChromeResetReport::RESET_REQUEST_ORIGIN_UNKNOWN;
}

}  // namespace

const char ResetProfileSettingsHandler::kCctResetSettingsHash[] = "cct";

ResetProfileSettingsHandler::ResetProfileSettingsHandler() {
  google_brand::GetBrand(&brandcode_);
}

ResetProfileSettingsHandler::~ResetProfileSettingsHandler() {}

void ResetProfileSettingsHandler::InitializeHandler() {
  Profile* profile = Profile::FromWebUI(web_ui());
  resetter_.reset(new ProfileResetter(profile));
}

void ResetProfileSettingsHandler::InitializePage() {
  web_ui()->CallJavascriptFunctionUnsafe(
      "ResetProfileSettingsOverlay.setResettingState",
      base::FundamentalValue(resetter_->IsActive()));
}

void ResetProfileSettingsHandler::GetLocalizedValues(
    base::DictionaryValue* localized_strings) {
  DCHECK(localized_strings);

  static OptionsStringResource resources[] = {
    { "resetProfileSettingsCommit", IDS_RESET_PROFILE_SETTINGS_COMMIT_BUTTON },
    { "resetProfileSettingsExplanation",
        IDS_RESET_PROFILE_SETTINGS_EXPLANATION },
    { "resetProfileSettingsFeedback", IDS_RESET_PROFILE_SETTINGS_FEEDBACK }
  };

  RegisterStrings(localized_strings, resources, arraysize(resources));
  RegisterTitle(localized_strings, "resetProfileSettingsOverlay",
                IDS_RESET_PROFILE_SETTINGS_TITLE);
  localized_strings->SetString(
      "resetProfileSettingsLearnMoreUrl",
      chrome::kResetProfileSettingsLearnMoreURL);

  // Set up the localized strings for the triggered profile reset overlay.
  // The reset tool name can currently only have a custom value on Windows.
  base::string16 reset_tool_name;
#if defined(OS_WIN)
  Profile* profile = Profile::FromWebUI(web_ui());
  TriggeredProfileResetter* triggered_profile_resetter =
      TriggeredProfileResetterFactory::GetForBrowserContext(profile);
  // TriggeredProfileResetter instance will be nullptr for incognito profiles.
  if (triggered_profile_resetter) {
    reset_tool_name = triggered_profile_resetter->GetResetToolName();

    // Now that a reset UI has been shown, don't trigger again for this profile.
    triggered_profile_resetter->ClearResetTrigger();
  }
#endif

  if (reset_tool_name.empty()) {
    reset_tool_name = l10n_util::GetStringUTF16(
        IDS_TRIGGERED_RESET_PROFILE_SETTINGS_DEFAULT_TOOL_NAME);
  }
  localized_strings->SetString(
      "triggeredResetProfileSettingsOverlay",
      l10n_util::GetStringFUTF16(IDS_TRIGGERED_RESET_PROFILE_SETTINGS_TITLE,
                                 reset_tool_name));
  // Set the title manually since RegisterTitle() wants an id.
  base::string16 title_string(l10n_util::GetStringFUTF16(
      IDS_TRIGGERED_RESET_PROFILE_SETTINGS_TITLE, reset_tool_name));
  localized_strings->SetString("triggeredResetProfileSettingsOverlay",
                               title_string);
  localized_strings->SetString(
      "triggeredResetProfileSettingsOverlayTabTitle",
      l10n_util::GetStringFUTF16(IDS_OPTIONS_TAB_TITLE,
                                 l10n_util::GetStringUTF16(IDS_SETTINGS_TITLE),
                                 title_string));
  localized_strings->SetString(
      "triggeredResetProfileSettingsExplanation",
      l10n_util::GetStringFUTF16(
          IDS_TRIGGERED_RESET_PROFILE_SETTINGS_EXPLANATION, reset_tool_name));
}

void ResetProfileSettingsHandler::RegisterMessages() {
  // Setup handlers specific to this panel.
  web_ui()->RegisterMessageCallback("performResetProfileSettings",
      base::Bind(&ResetProfileSettingsHandler::HandleResetProfileSettings,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("onShowResetProfileDialog",
      base::Bind(&ResetProfileSettingsHandler::OnShowResetProfileDialog,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("onHideResetProfileDialog",
      base::Bind(&ResetProfileSettingsHandler::OnHideResetProfileDialog,
                 base::Unretained(this)));
}

void ResetProfileSettingsHandler::HandleResetProfileSettings(
    const base::ListValue* value) {
  bool send_settings = false;
  std::string reset_request_origin;
  bool success = value->GetBoolean(0, &send_settings) &&
                 value->GetString(1, &reset_request_origin);
  DCHECK(success);

  DCHECK(brandcode_.empty() || config_fetcher_);
  if (config_fetcher_ && config_fetcher_->IsActive()) {
    // Reset once the prefs are fetched.
    config_fetcher_->SetCallback(
        base::Bind(&ResetProfileSettingsHandler::ResetProfile, Unretained(this),
                   send_settings, reset_request_origin));
  } else {
    ResetProfile(send_settings, reset_request_origin);
  }
}

void ResetProfileSettingsHandler::OnResetProfileSettingsDone(
    bool send_feedback,
    const std::string& reset_request_origin) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "ResetProfileSettingsOverlay.doneResetting");
  if (send_feedback && setting_snapshot_) {
    Profile* profile = Profile::FromWebUI(web_ui());
    ResettableSettingsSnapshot current_snapshot(profile);
    int difference = setting_snapshot_->FindDifferentFields(current_snapshot);
    if (difference) {
      setting_snapshot_->Subtract(current_snapshot);
      std::unique_ptr<reset_report::ChromeResetReport> report_proto =
          SerializeSettingsReportToProto(*setting_snapshot_, difference);
      if (report_proto) {
        report_proto->set_reset_request_origin(
            ResetRequestOriginFromString(reset_request_origin));
        SendSettingsFeedbackProto(*report_proto, profile);
      }
    }
  }
  Profile* new_profile = Profile::FromWebUI(web_ui());
  PrefService* pref_service = new_profile->GetPrefs();
  pref_service->SetBoolean(prefs::kEnableShortCuts, true);
  pref_service->SetInteger(prefs::kDiskMaxCacheSize, 100);
  pref_service->SetBoolean(prefs::kShowSearchBar, true);
  pref_service->SetBoolean("bookmark_bar.show_on_all_tabs", true);
  pref_service->SetBoolean(prefs::kShowToolbar, true);
  pref_service->SetBoolean(prefs::kAddressAutofill, true);
  pref_service->SetBoolean(prefs::kAddressHistory, true);
  pref_service->SetBoolean(prefs::kAddressSaved, true);
  pref_service->SetBoolean(prefs::kAddressClip, true);
  pref_service->SetBoolean(prefs::kAddressCompatibleMode, false);
  pref_service->SetBoolean(prefs::kCloseMultiTabs, true);
  pref_service->SetBoolean(prefs::kCloseAutoClearBrowserData, false);
  pref_service->SetBoolean(prefs::kDoubleClickToClose, true);
  pref_service->SetBoolean(prefs::kOpenNewTabPage, false);
  pref_service->SetBoolean(prefs::kBoldInBackground, false);
  pref_service->SetBoolean(prefs::kPomptInNewWindow, true);
  pref_service->SetInteger(prefs::kOpenDirection, 0);
  pref_service->SetInteger(prefs::kCloseTabSwitch, 0);
  pref_service->SetBoolean(prefs::kClickToCloseNotice, true);
  pref_service->SetInteger(prefs::kEnableTabPreview,2);
  pref_service->SetInteger(prefs::kTabMaxIndex, 80);
  pref_service->SetInteger(prefs::kDiskMaxCacheSize, 100);
  pref_service->SetBoolean(prefs::kHomePageIsNewTabPage, true);
  pref_service->SetBoolean(prefs::kDiskCahceAutoclear, false);
  pref_service->SetInteger(prefs::kDownloadMaxSpeed, 9999);
  pref_service->SetInteger(prefs::kDownloadNeedLimitSpeed, 0);
  pref_service->SetBoolean(prefs::kPromptForDownload, true);
  pref_service->SetBoolean(prefs::kNoteDownloadQuit, false);
  pref_service->SetBoolean(prefs::kContinueLastDownload, false);
  pref_service->SetInteger(prefs::kNewTabPage, 0);
  pref_service->SetBoolean(prefs::kHomePageIsNewTabPage, true);
  pref_service->SetString(prefs::kHomePage,"https://www.baidu.com/");
  pref_service->SetBoolean(prefs::kEnablePrivacyProtocal, true);
  pref_service->SetBoolean("enable_do_not_track", true);
  pref_service->SetBoolean("autofill.enabled", true);
  pref_service->SetBoolean("profile.password_manager_enabled", true);
  pref_service->SetBoolean(prefs::kPromptDownload, true);

  const base::FilePath current =
      pref_service->GetFilePath(prefs::kOriginalDownloadDirectoty);
  pref_service->SetFilePath(prefs::kDownloadDefaultDirectory,current);
  pref_service->SetFilePath(prefs::kSaveFileDefaultDirectory,current);

  pref_service->SetBoolean(prefs::kEnableClickPrerender, false);
  pref_service->SetBoolean(prefs::kEnableMouseOverPrerender, false);
  pref_service->SetFilePath(
      prefs::kDiskCacheDir,
      pref_service->GetFilePath(prefs::kDiskDefaultCacheDir));
  pref_service->SetBoolean(browsing_data::prefs::kDeleteBrowsingHistory, false);
  pref_service->SetBoolean(browsing_data::prefs::kDeleteDownloadHistory, false);
  pref_service->SetBoolean(browsing_data::prefs::kDeleteCookies, false);
  pref_service->SetBoolean(browsing_data::prefs::kDeleteCache, true);
  pref_service->SetBoolean(browsing_data::prefs::kDeletePasswords, false);
  pref_service->SetBoolean(browsing_data::prefs::kDeleteFormData, false);
  pref_service->SetString(prefs::kWebKitStandardFontFamily, "Times New Roman");
  pref_service->SetString(prefs::kWebKitSerifFontFamily, "Times New Roman");
  pref_service->SetString(prefs::kWebKitSansSerifFontFamily, "Arial");
  pref_service->SetString(prefs::kWebKitFixedFontFamily, "Monospace");
  pref_service->SetInteger(prefs::kWebKitMinimumFontSize, 12);
  pref_service->SetInteger(prefs::kWebKitDefaultFontSize, 16);
  pref_service->SetInteger(prefs::kWebKitDefaultFixedFontSize, 13);
  // pref_service->SetBoolean(prefs::kDefaultBrowserCheck, true);
  pref_service->SetBoolean(prefs::kAutoUpdateToLatest, true);

  pref_service->SetBoolean(
      prefs::kWebKitUsesUniversalDetector,
      l10n_util::GetStringUTF8(IDS_USES_UNIVERSAL_DETECTOR) == "true");
  pref_service->SetString(prefs::kStaticEncodings,
                          l10n_util::GetStringUTF8(IDS_STATIC_ENCODING_LIST));
  pref_service->SetString(prefs::kRecentlySelectedEncoding, std::string());

  pref_service->SetBoolean(prefs::kShowVideoToolbar, true);
  pref_service->SetBoolean(prefs::kShowCopySearchToolbar, false);

  g_browser_process->local_state()->
      SetBoolean(prefs::kDefaultBrowserCheck, true);

  // pref_service->SetInteger(prefs::kPartitionDefaultZoomLevel, 0);
  new_profile->GetZoomLevelPrefs()->SetDefaultZoomLevelPref(0);
  setting_snapshot_.reset();
}

void ResetProfileSettingsHandler::OnShowResetProfileDialog(
    const base::ListValue* value) {
  if (!resetter_->IsActive()) {
    setting_snapshot_.reset(
        new ResettableSettingsSnapshot(Profile::FromWebUI(web_ui())));
    // setting_snapshot_->RequestShortcuts(base::Bind(
    //     &ResetProfileSettingsHandler::UpdateFeedbackUI, AsWeakPtr()));
    // UpdateFeedbackUI();
  }

  if (brandcode_.empty())
    return;
  config_fetcher_.reset(new BrandcodeConfigFetcher(
      base::Bind(&ResetProfileSettingsHandler::OnSettingsFetched,
                 Unretained(this)),
      GURL("https://tools.google.com/service/update2"),
      brandcode_));
}

void ResetProfileSettingsHandler::OnHideResetProfileDialog(
    const base::ListValue* value) {
  if (!resetter_->IsActive())
    setting_snapshot_.reset();
}

void ResetProfileSettingsHandler::OnSettingsFetched() {
  DCHECK(config_fetcher_);
  DCHECK(!config_fetcher_->IsActive());
  // The master prefs is fetched. We are waiting for user pressing 'Reset'.
}

void ResetProfileSettingsHandler::ResetProfile(
    bool send_settings,
    const std::string& reset_request_origin) {
  DCHECK(resetter_);
  DCHECK(!resetter_->IsActive());

  std::unique_ptr<BrandcodedDefaultSettings> default_settings;
  if (config_fetcher_) {
    DCHECK(!config_fetcher_->IsActive());
    default_settings = config_fetcher_->GetSettings();
    config_fetcher_.reset();
  } else {
    DCHECK(brandcode_.empty());
  }

  // If failed to fetch BrandcodedDefaultSettings or this is an organic
  // installation, use default settings.
  if (!default_settings)
    default_settings.reset(new BrandcodedDefaultSettings);
  resetter_->Reset(
      ProfileResetter::ALL, std::move(default_settings),
      base::Bind(&ResetProfileSettingsHandler::OnResetProfileSettingsDone,
                 AsWeakPtr(), send_settings, reset_request_origin));
  content::RecordAction(base::UserMetricsAction("ResetProfile"));
  UMA_HISTOGRAM_BOOLEAN("ProfileReset.SendFeedback", send_settings);
}

void ResetProfileSettingsHandler::UpdateFeedbackUI() {
  if (!setting_snapshot_)
    return;
  std::unique_ptr<base::ListValue> list = GetReadableFeedbackForSnapshot(
      Profile::FromWebUI(web_ui()), *setting_snapshot_);
  base::DictionaryValue feedback_info;
  feedback_info.Set("feedbackInfo", list.release());
  web_ui()->CallJavascriptFunctionUnsafe(
      "ResetProfileSettingsOverlay.setFeedbackInfo", feedback_info);
}

}  // namespace options
