// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_THEME_GALLERY_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_THEME_GALLERY_HANDLER_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Browser;
class Profile;

// Handles actions on Welcome page.
class ThemeGalleryHandler : public content::WebUIMessageHandler,
                                                    public content::NotificationObserver {
 public:
  explicit ThemeGalleryHandler(content::WebUI* web_ui);
  ~ThemeGalleryHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

 private:
   // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void HandleInstallTheme(const base::ListValue* args);
  void HandleGetCurrentTheme(const base::ListValue* args);


  Browser* GetBrowser();
  Profile* GetProfile() const;
  void SetTheme(std::string themeID);

  content::NotificationRegistrar registrar_;
  // bool installing_;

  DISALLOW_COPY_AND_ASSIGN(ThemeGalleryHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_THEME_GALLERY_HANDLER_H_
