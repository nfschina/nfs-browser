// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/preinstall_extension/preinstall_extension_service.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/renderer/render_frame_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/browser_thread_impl.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/pre_install/preinstall_crx_info.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"


#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_finder.h"

namespace AdblockManager{
// 广告误拦截 白名单
const char* whitelist_domain[] = {
    "baomihua.com",
    "baidu.com",
    "iqiyi.com",
    "hdfybjy.com",
    "mop.com", // for memory reason bug#345857
    "lnzsks.com", // #347084
    "tv.youku.com",
    "tvs.youku.com",// #0347196
};

content::RenderFrameHost* GetMainRenderFrameHost(Profile* profile);

void DoPreWork(Profile* profile);
}

PreinstallExtensionService::PreinstallExtensionService(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)),
      extension_registry_observer_(this),
      registrar_(new content::NotificationRegistrar) {
 extension_registry_observer_.Add(extensions::ExtensionRegistry::Get(profile_));
}

PreinstallExtensionService::~PreinstallExtensionService() {
  bool registered = registrar_->IsRegistered(
         this, extensions::NOTIFICATION_EXTENSION_HOST_CREATED,
         content::Source<Profile>(profile_));
   if (registered)  {
       registrar_->Remove(this,
               extensions::NOTIFICATION_EXTENSION_HOST_CREATED,
               content::Source<Profile>(profile_));
  }
}

void PreinstallExtensionService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension && extension->id() == extensions::GetMouseGestureID()) {

    std::string protocal = "chrome-extension://";
    GURL url(protocal + extensions::GetMouseGestureID() + "/background.html");
    chrome::CreateHideWebContent(browser_context, url);
  }
}

void PreinstallExtensionService::OnExtensionReady(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension && extension->id() == extensions::GetAdBlockID())  {
    bool registered = registrar_->IsRegistered(
          this, extensions::NOTIFICATION_EXTENSION_HOST_CREATED,
          content::Source<Profile>(profile_));
    if (registered)  {
        return;
    }
   registrar_->Add(this,
                extensions::NOTIFICATION_EXTENSION_HOST_CREATED,
                content::Source<Profile>(profile_));
  }
}

void PreinstallExtensionService::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(extensions::NOTIFICATION_EXTENSION_HOST_CREATED, type);
  const extensions::ExtensionHost* host =
      content::Details<extensions::ExtensionHost>(details).ptr();
 
  if (host->extension_id().compare(extensions::GetAdBlockID()) == 0)  {
     content::BrowserThread::PostDelayedTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&AdblockManager::DoPreWork,
                                              profile_),
                                   base::TimeDelta::FromMilliseconds(1000));
   }
}

content::RenderFrameHost* AdblockManager::GetMainRenderFrameHost(Profile* profile) {
  const std::string adblock_id = extensions::GetAdBlockID();

  extensions::ProcessManager* process_manager = extensions::ProcessManager::Get(profile);
  if (!process_manager) {
    return NULL;
  }
  extensions::ExtensionHost* extension_host = process_manager->GetBackgroundHostForExtension(adblock_id);
  if (!extension_host) {
    return NULL;
  }
  content::RenderViewHost* rvh = extension_host->render_view_host();
  if (!rvh || !rvh->IsRenderViewLive()) {
    return NULL;
  }

  return rvh->GetMainFrame();
}

void AdblockManager::DoPreWork(Profile* profile) {
  content::RenderFrameHost* host = GetMainRenderFrameHost(profile);
  if (host == NULL)  {
    return;
  }

  // whitelist
  const std::string jsFunc1 = "addWhitelistDomainCDOS";
  for (size_t i = 0; i < arraysize(whitelist_domain); ++i)  {
    char jsCode[512];
    sprintf(jsCode, "%s(\"%s\")", jsFunc1.c_str(), whitelist_domain[i]);

    host->ExecuteJavaScript(base::ASCIIToUTF16(jsCode));
  }

  // subscription
  const std::string jsFunc2 = "addSubscriptionCDOS";
  host->ExecuteJavaScript(base::ASCIIToUTF16(jsFunc2.c_str()));
}
