// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_ABOUT_UI_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_ABOUT_UI_HANDLER_H_

#include <string>
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_ui_message_handler.h"

class AboutUIHandler : public content::WebUIMessageHandler {
public:
  explicit AboutUIHandler(content::WebUI* web_ui);
  ~AboutUIHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

private:
  void OpenChromiumUrl(const base::ListValue* args);
  void OpenSourceUrl(const base::ListValue* args);
  void OpenHelpUrl(const base::ListValue* args);

  Profile* profile_;
  Browser* browser_;

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<AboutUIHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AboutUIHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_ABOUT_UI_HANDLER_H_
