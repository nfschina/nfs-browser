// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/about_nfs/about_ui_handler.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"

AboutUIHandler::AboutUIHandler(content::WebUI* web_ui)
    : profile_(Profile::FromWebUI(web_ui))
    , weak_ptr_factory_(this) {
  browser_ = chrome::FindBrowserWithProfile(profile_);
}

AboutUIHandler::~AboutUIHandler() {
}

void AboutUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "openChromiumUrl", base::Bind(&AboutUIHandler::OpenChromiumUrl,
                               base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "openSourceUrl", base::Bind(&AboutUIHandler::OpenSourceUrl,
                               base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "openHelpUrl", base::Bind(&AboutUIHandler::OpenHelpUrl,
                               base::Unretained(this)));
}

void AboutUIHandler::OpenChromiumUrl(const base::ListValue* args) {
  chrome::ShowChromiumUrl(browser_);
}

void AboutUIHandler::OpenSourceUrl(const base::ListValue* args) {
  chrome::ShowSourceUrl(browser_);
}

void AboutUIHandler::OpenHelpUrl(const base::ListValue* args) {
  chrome::ShowHelpUrl(browser_);
}
