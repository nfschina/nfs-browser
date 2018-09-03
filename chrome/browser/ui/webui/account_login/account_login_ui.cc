// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/account_login/account_login_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/account_login/account_login_handler.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/browser_resources.h"

namespace {

content::WebUIDataSource* CreateAboutUINewHTMLSource(Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIAccountLoginHost);

  source->AddResourcePath("js/account_login.js", IDR_ACCOUNT_LOGIN_JS);

  source->SetDefaultResource(IDR_ACCOUNT_LOGIN_HTML);

  return source;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// AccountLoginUI
//
///////////////////////////////////////////////////////////////////////////////

AccountLoginUI::AccountLoginUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  content::WebUIDataSource* source = CreateAboutUINewHTMLSource(profile);
  content::WebUIDataSource::Add(profile, source);

  web_ui->AddMessageHandler(new AccountLoginHandler(web_ui));
}

AccountLoginUI::~AccountLoginUI() {
}
