// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_ACCOUNT_LOGIN_ACCOUNT_LOGIN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_ACCOUNT_LOGIN_ACCOUNT_LOGIN_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class AccountLoginUI : public content::WebUIController {
public:
  explicit AccountLoginUI(content::WebUI* web_ui);
  ~AccountLoginUI() override;

private:
  DISALLOW_COPY_AND_ASSIGN(AccountLoginUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_ACCOUNT_LOGIN_ACCOUNT_LOGIN_UI_H_
