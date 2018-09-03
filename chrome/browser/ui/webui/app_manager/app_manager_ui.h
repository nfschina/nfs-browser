// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_APP_MANAGER_UI_H_
#define CHROME_BROWSER_UI_WEBUI_APP_MANAGER_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/base/layout.h"
#include "url/gurl.h"

class Profile;

class AppManagerUI : public content::WebUIController {
 public:
  AppManagerUI(content::WebUI* web_ui);
  ~AppManagerUI() override;  

 private:
  Profile* GetProfile() const;

  DISALLOW_COPY_AND_ASSIGN(AppManagerUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_APP_MANAGER_UI_H_
