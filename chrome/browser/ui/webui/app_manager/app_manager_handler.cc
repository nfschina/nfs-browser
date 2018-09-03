// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/app_manager/app_manager_handler.h"

#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/profile_chooser_constants.h"
#include "chrome/browser/ui/views/web_content_dialog_view.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/update_query_params.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "third_party/curl/src/include/curl/curl.h"
#include "third_party/curl/src/include/curl/easy.h"
#include "third_party/zlib/google/zip.h"

AppManagerHandler::AppManagerHandler(content::WebUI* web_ui)
    : extension_registry_observer_(this),
      weak_factory_(this) {
          extension_registry_observer_.Add(extensions::ExtensionRegistry::Get(Profile::FromWebUI(web_ui)));
}

AppManagerHandler::~AppManagerHandler() {
}

void AppManagerHandler::OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) {
   NotifyExtensionEvent(extension->id(), EXTENSION_EVENT_ENABLED);
}

void AppManagerHandler::OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionInfo::Reason reason) {
   NotifyExtensionEvent(extension->id(), EXTENSION_EVENT_DISABLED);
}

void AppManagerHandler::OnExtensionInstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension, bool is_update) {
  NotifyExtensionEvent(extension->id(), EXTENSION_EVENT_INSTALLED);
}

void AppManagerHandler::OnExtensionUninstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension, extensions::UninstallReason reason) {
  web_ui()->CallJavascriptFunctionUnsafe("appManager.onUninstalled", base::StringValue(extension->id()));
}

void AppManagerHandler::NotifyExtensionEvent(std::string extension_id, EXTENSION_EVENT event) {
  info_generator_.reset(new extensions::ExtensionInfoGenerator(GetProfile()));
  info_generator_->CreateExtensionInfo(extension_id,
      base::Bind(&AppManagerHandler:: NotifyExtensionEventHelper, base::Unretained(this), event));
}

void AppManagerHandler::NotifyExtensionEventHelper(
    EXTENSION_EVENT event,
    extensions::ExtensionInfoGenerator::ExtensionInfoList infos) {
        if(!infos.size()) {
          return;
        }
        std::string function_name;
        switch(event) {
          case EXTENSION_EVENT_INSTALLED:
            function_name = "appManager.onInstalled";
            break;
          case EXTENSION_EVENT_ENABLED:
            function_name = "appManager.onEnabled";
            break;
          case EXTENSION_EVENT_DISABLED:
            function_name = "appManager.onDisabled";
            break;
          default:
            return;
        }
        web_ui()->CallJavascriptFunctionUnsafe(function_name,
                  *(extensions::api::developer_private::GetExtensionsInfo::Results::Create(infos).get()));
}

void AppManagerHandler::HandleGetExtensionsInfo(const base::ListValue* args) {
   info_generator_.reset(new extensions::ExtensionInfoGenerator(GetProfile()));
      info_generator_->CreateExtensionsInfo(
      true,
      true,
      base::Bind(&AppManagerHandler::OnInfosGenerated,
                 base::Unretained(this)));
}

void AppManagerHandler::OnInfosGenerated(
    extensions::ExtensionInfoGenerator::ExtensionInfoList list) {
  web_ui()->CallJavascriptFunctionUnsafe("appManager.onInfosGenerated",
              *(extensions::api::developer_private::GetExtensionsInfo::Results::Create(list).get()));

  base::FilePath pepper_flash_path;
  PathService::Get(chrome::DIR_PEPPER_FLASH_PLUGIN, &pepper_flash_path);
  bool installed = !base::IsDirectoryEmpty(pepper_flash_path);
  base::DictionaryValue dictionary;
  dictionary.SetBoolean("has_installed", installed);
  dictionary.SetString("id", "pepperFlash");
  web_ui()->CallJavascriptFunctionUnsafe("appManager.OnInstallstatus", dictionary);
}

void AppManagerHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "enable", base::Bind(&AppManagerHandler::HandleEnable,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "disable", base::Bind(&AppManagerHandler::HandleDisable,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "install", base::Bind(&AppManagerHandler::HandleInstall,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "uninstall", base::Bind(&AppManagerHandler::HandleUninstall,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "getExtensionsInfo", base::Bind(&AppManagerHandler::HandleGetExtensionsInfo,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "more", base::Bind(&AppManagerHandler::HandleMore,
                                         base::Unretained(this)));
}

void AppManagerHandler::InstallPepperFlash() {
  base::FilePath download_path, tmp_path, pepper_flash_base_path, unzip_path;
  PathService::Get(base::DIR_TEMP, &tmp_path);
  PathService::Get(chrome::DIR_INTERNAL_PLUGINS, &pepper_flash_base_path);
#if defined(OS_WIN)
  download_path = tmp_path.Append(L"pepperFlash.zip");
  unzip_path = tmp_path.Append(L"PepperFlash");
#else
  download_path = tmp_path.Append("pepperFlash.zip");
  unzip_path = tmp_path.Append("PepperFlash");
#endif
  std::string url = chrome::kWebStoreURLOrigin;
  std::string os_arch = update_client::UpdateQueryParams::GetOS();
  os_arch += update_client::UpdateQueryParams::GetArch();
  if(os_arch == "linuxx86") {
    os_arch = "linux";
  } else if (os_arch == "linuxx64") {
    os_arch = "linux_x64";
  } else if (os_arch == "winx86") {
    os_arch = "win";
  } else if (os_arch == "winx64") {
    os_arch = "win_x64";
  } else {
    return;
  }

  url += "/apps/pepperFlash/";
  url += os_arch;
  url += "/pepperFlash.zip";

  #if defined(OS_WIN)
    std::string download_path_str = base::WideToUTF8(download_path.value());
  #else
    std::string download_path_str = download_path.value();
  #endif

  CURL *curl;
  FILE *fp;
  CURLcode res;
  // Call curl_global_init() to initialize libcurl.
  res = curl_global_init(CURL_GLOBAL_ALL);

  // Call curl_easy_init() to get the pointer of easy interface.
  curl = curl_easy_init();
  fp = fopen(download_path_str.c_str(), "wb");
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FILE, fp);

  res = curl_easy_perform(curl);
  if (CURLE_OK == res) {
    fclose(fp);
    zip::Unzip(download_path, tmp_path);
    base::CopyDirectory(unzip_path, pepper_flash_base_path, true);
    base::DeleteFile(download_path, true);
    base::DeleteFile(unzip_path, true);
  } else {
    // If download failed, delete the temporary file.
    fclose(fp);
    base::DeleteFile(download_path, true);
  }

  // always cleanup.
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void AppManagerHandler::PepperFlashInstalled() {
  base::FilePath pepper_flash_path;
  PathService::Get(chrome::DIR_PEPPER_FLASH_PLUGIN, &pepper_flash_path);
  bool success = !base::IsDirectoryEmpty(pepper_flash_path);
  base::DictionaryValue dictionary;
  dictionary.SetBoolean("success", success);
  web_ui()->CallJavascriptFunctionUnsafe("appManager.onPepperFlashInstalled", dictionary);
}

void AppManagerHandler::UninstallPepperFlash() {
  base::FilePath  pepper_flash_path;
  PathService::Get(chrome::DIR_PEPPER_FLASH_PLUGIN, &pepper_flash_path);
  base::DeleteFile(pepper_flash_path, true);
}

void AppManagerHandler::PepperFlashUninstalled() {
  web_ui()->CallJavascriptFunctionUnsafe("appManager.onUninstalled",
      base::StringValue("pepperFlash"));
}

void AppManagerHandler::HandleInstall(const base::ListValue* args) {
  std::string name;
  args->GetString(0, &name);
  if (name == "pepperFlash") {
    content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&AppManagerHandler::InstallPepperFlash, weak_factory_.GetWeakPtr()),
      base::Bind(&AppManagerHandler::PepperFlashInstalled, weak_factory_.GetWeakPtr()));
    return;
  }
  scoped_refptr<extensions::CrxInstaller> installer(extensions::CrxInstaller::CreateSilent(
        extensions::ExtensionSystem::Get(GetProfile())->extension_service()));
  base::FilePath crx_path;
  if (PathService::Get(chrome::DIR_APP, &crx_path)) {
    crx_path = crx_path.AppendASCII(chrome::kExtensionDir);
    crx_path = crx_path.AppendASCII(name);
    // printf("###install crx_path=%s\n", crx_path.AsUTF8Unsafe().c_str());
    installer->set_allow_silent_install(true);
    installer->set_install_immediately(true);
    installer->InstallCrx(crx_path);
  }
}

void AppManagerHandler::HandleEnable(const base::ListValue* args) {
  std::string id;
  args->GetString(0, &id);
  ExtensionService* service =
          extensions::ExtensionSystem::Get(GetProfile())->extension_service();
  if (!service->IsExtensionEnabled(id)) {
    service->EnableExtension(id);
  }
}

void AppManagerHandler::HandleDisable(const base::ListValue* args) {
  std::string id;
  args->GetString(0, &id);
  ExtensionService* service =
          extensions::ExtensionSystem::Get(GetProfile())->extension_service();
  if (service->IsExtensionEnabled(id)) {
    service->DisableExtension(id, extensions::Extension::DISABLE_USER_ACTION);
  }
}

void AppManagerHandler::HandleUninstall(const base::ListValue* args) {
  std::string id;
  args->GetString(0, &id);
  if (id == "pepperFlash") {
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::FILE, FROM_HERE,
        base::Bind(&AppManagerHandler::UninstallPepperFlash, weak_factory_.GetWeakPtr()),
        base::Bind(&AppManagerHandler::PepperFlashUninstalled, weak_factory_.GetWeakPtr()));
    return;
  }
  ExtensionService* service =
      extensions::ExtensionSystem::Get(GetProfile())->extension_service();
  service->UninstallExtension(id,
                              extensions::UNINSTALL_REASON_USER_INITIATED,
                              base::Bind(&base::DoNothing), nullptr);
}

void AppManagerHandler::HandleMore(const base::ListValue* args) {
  chrome::ShowWebStore(GetBrowser());
  nfsbrowser::WebContentDialogView::Hide(web_ui()->GetWebContents());
}

Profile* AppManagerHandler::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}

Browser* AppManagerHandler::GetBrowser() {
  Browser* browser = chrome::FindBrowserWithProfile(GetProfile());
  return browser;
}
