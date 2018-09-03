// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_APP_MANAGER_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_APP_MANAGER_HANDLER_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/extensions/api/developer_private/extension_info_generator.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/extension_registry_observer.h"

class Browser;
class Profile;

namespace extensions {
class ExtensionRegistry;
class ExtensionPrefs;
}

class AppManagerHandler : public content::WebUIMessageHandler,
                                                     public extensions::ExtensionRegistryObserver {
 public:
  explicit AppManagerHandler(content::WebUI* web_ui);
  ~AppManagerHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

 private:
  enum EXTENSION_EVENT
  {
    EXTENSION_EVENT_INSTALLED,
    EXTENSION_EVENT_UNINSTALLED,
    EXTENSION_EVENT_ENABLED,
    EXTENSION_EVENT_DISABLED,
  };
  // ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionInfo::Reason reason) override;
  void OnExtensionInstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension, bool is_update) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const extensions::Extension* extension, extensions::UninstallReason reason) override;

  void  NotifyExtensionEvent(std::string extension_id, EXTENSION_EVENT event);
  void  NotifyExtensionEventHelper(EXTENSION_EVENT event,
            extensions::ExtensionInfoGenerator::ExtensionInfoList infos);

  void HandleEnable(const base::ListValue* args);
  void HandleDisable(const base::ListValue* args);
  void InstallPepperFlash();
  void PepperFlashInstalled();
  void UninstallPepperFlash();
  void PepperFlashUninstalled();
  void HandleInstall(const base::ListValue* args);
  void HandleUninstall(const base::ListValue* args);
  void HandleGetExtensionsInfo(const base::ListValue* args);
  void HandleMore(const base::ListValue* args);

  void OnInfosGenerated(
    extensions::ExtensionInfoGenerator::ExtensionInfoList list);

  Browser* GetBrowser();
  Profile* GetProfile() const;

  ScopedObserver<extensions::ExtensionRegistry, extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  std::unique_ptr<extensions::ExtensionInfoGenerator> info_generator_;

  base::WeakPtrFactory<AppManagerHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppManagerHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_APP_MANAGER_HANDLER_H_
