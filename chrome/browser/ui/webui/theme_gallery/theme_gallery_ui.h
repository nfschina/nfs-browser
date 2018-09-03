// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_THEME_GALLERY_UI_H_
#define CHROME_BROWSER_UI_WEBUI_THEME_GALLERY_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/base/layout.h"
#include "url/gurl.h"

class Profile;

// namespace base {
// class RefCountedMemory;
// }

class ThemeGalleryUI : public content::WebUIController {
 public:
  ThemeGalleryUI(content::WebUI* web_ui);
  ~ThemeGalleryUI() override;

  // static base::RefCountedMemory* GetFaviconResourceBytes(
  //    ui::ScaleFactor scale_factor);

 private:
  Profile* GetProfile() const;

  DISALLOW_COPY_AND_ASSIGN(ThemeGalleryUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_THEME_GALLERY_UI_H_
