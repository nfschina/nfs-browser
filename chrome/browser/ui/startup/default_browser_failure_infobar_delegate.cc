// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/default_browser_failure_infobar_delegate.h"

#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/user_metrics.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/vector_icons_public.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "components/variations/variations_associated_data.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#endif

#include "chrome/browser/ui/views/importer/import_dialog_view.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"

#include "chrome/common/pref_names.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/pref_names.h"

namespace chrome {
// static
void DefaultBrowserFailureInfoBarDelegate::Create(InfoBarService* infobar_service,
                                           Profile* profile) {
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new DefaultBrowserFailureInfoBarDelegate(profile))));
}

DefaultBrowserFailureInfoBarDelegate::DefaultBrowserFailureInfoBarDelegate(Profile* profile)
    : ConfirmInfoBarDelegate(),
      profile_(profile),
      action_taken_(false),
      weak_factory_(this) {
  // We want the info-bar to stick-around for few seconds and then be hidden
  // on the next navigation after that.
      //base::TimeDelta::FromSeconds(8));
}

DefaultBrowserFailureInfoBarDelegate::~DefaultBrowserFailureInfoBarDelegate() {
}

infobars::InfoBarDelegate::Type DefaultBrowserFailureInfoBarDelegate::GetInfoBarType()
    const {
  return WARNING_TYPE;
}

infobars::InfoBarDelegate::InfoBarIdentifier
DefaultBrowserFailureInfoBarDelegate::GetIdentifier() const {
  return CHECK_UPDATE_INFOBAR_DELEGATE;
}

int DefaultBrowserFailureInfoBarDelegate::GetIconId() const {
  return IDR_PRODUCT_LOGO_16;
}

void DefaultBrowserFailureInfoBarDelegate::InfoBarDismissed() {
  action_taken_ = true;
  // |profile_| may be null in tests.
  if (profile_) {
    profile_->GetPrefs()->SetBoolean(prefs::kShouldShowImportDialog, false);
  }
}

base::string16 DefaultBrowserFailureInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_DEFAULT_BROWSER_FAILURE_INFOBAR_SHORT_TEXT);
}

int DefaultBrowserFailureInfoBarDelegate::GetButtons() const {
  return InfoBarButton::BUTTON_NONE;
}

base::string16 DefaultBrowserFailureInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  //DCHECK_EQ(BUTTON_OK, button);
  return button == BUTTON_OK ? l10n_util::GetStringUTF16(IDS_CDOS_CHECK_UPDATE_TITLE) : l10n_util::GetStringUTF16(IDS_CDOS_CHECK_UPDATE_CANCEL_TITLE);
}

bool DefaultBrowserFailureInfoBarDelegate::Accept() {
  Browser* browser = chrome::FindLastActive();
  if (!browser)
    return false;
  return true;
}

bool DefaultBrowserFailureInfoBarDelegate::Cancel() {
  //profile_->GetPrefs()->SetString(prefs::kBrowserLastUpdateVersion, version_);
  return true;
}

}  // namespace chrome