// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_STARTUP_CHECK_UPDATE_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_STARTUP_CHECK_UPDATE_INFOBAR_DELEGATE_H_

#include "base/feature_list.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/shell_integration.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include <string>

class Profile;

namespace chrome {
// The delegate for the infobar shown when Chrome need to import favorites.
// Ownership of the delegate is given to the infobar itself, the lifetime of
// which is bound to the containing WebContents.
class CheckUpdateInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Creates a default browser infobar and delegate and adds the infobar to
  // |infobar_service|.
  static void Create(InfoBarService* infobar_service, Profile* profile, std::string version,
                          std::string name , std::string url);

 protected:
  explicit CheckUpdateInfoBarDelegate(Profile* profile, std::string version,
                     std::string name, std::string url);
  ~CheckUpdateInfoBarDelegate() override;

 private:
  // Possible user interactions with the import favorites info bar.
  // Do not modify the ordering as it is important for UMA.
  enum InfoBarUserInteraction {
    // The user clicked the "OK" (i.e., "Set as default") button.
    ACCEPT_INFO_BAR = 0,
    // The cancel button is deprecated.
    //CANCEL_INFO_BAR = 1,
    // The user did not interact with the info bar.
    IGNORE_INFO_BAR = 2,
    // The user explicitly closed the infobar.
    DISMISS_INFO_BAR = 3,
    NUM_INFO_BAR_USER_INTERACTION_TYPES
  };

  // ConfirmInfoBarDelegate:
  Type GetInfoBarType() const override;
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  int GetIconId() const override;
  void InfoBarDismissed() override;
  base::string16 GetMessageText() const override;
  //int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;

  // The WebContents's corresponding profile.
  Profile* profile_;

  scoped_refptr<base::SingleThreadTaskRunner> thread_task_runner_;

  // Indicates if the user interacted with the infobar.
  bool action_taken_;

  //browser version.
  std::string version_;

  //browser package name.
  std::string name_;

  //url to fetch browser package.
  std::string url_;

  // Used to delay the expiration of the info-bar.
  base::WeakPtrFactory<CheckUpdateInfoBarDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CheckUpdateInfoBarDelegate);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_STARTUP_CHECK_UPDATE_INFOBAR_DELEGATE_H_