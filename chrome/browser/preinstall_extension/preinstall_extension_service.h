// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ADBLOCK_ADBLOCK_MANAGER_H_
#define CHROME_BROWSER_ADBLOCK_ADBLOCK_MANAGER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/scoped_observer.h"

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/extension_registry_observer.h"

class Profile;

namespace content {
  class RenderFrameHost;
};

class PreinstallExtensionService :  public extensions::ExtensionRegistryObserver,
                        public content::NotificationObserver {
 public:
  explicit PreinstallExtensionService(content::BrowserContext* context);
  ~PreinstallExtensionService() override;

 private:

  void OnExtensionLoaded(content::BrowserContext* browser_context, 
                          const extensions::Extension* extension) override;

  // ExtensionRegistryObserver implementation.
  void OnExtensionReady(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  Profile* profile_;

  // Listen to extension load, unloaded notifications.
  ScopedObserver<extensions::ExtensionRegistry, extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  std::unique_ptr<content::NotificationRegistrar> registrar_;
  
  DISALLOW_COPY_AND_ASSIGN(PreinstallExtensionService);
};
#endif  // CHROME_BROWSER_ADBLOCK_ADBLOCK_MANAGER_H_
