// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/update_nfs/update_ui_nfs.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/update_nfs/update_handle.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

using content::BrowserContext;
using content::BrowserThread;
using content::DownloadManager;
using content::WebUIController;
using content::WebUIDataSource;
using content::WebContents;

namespace {

const struct Resource{
  const char* filename;
  int identifier;
  const char* mime_type;
} kResources[] = {
    {"update_nfs.js", IDR_UPDATE_CDOS_JS, "application/javascript"},
    {"jquery.js", IDR_UPDATE_CDOS_JQUERY, "application/javascript"},
    {"img/bg.png", IDR_UPDATE_CDOS_BG, "image/png"},
    {"img/logo.png", IDR_UPDATE_CDOS_LOGO, "image/png"},
    {"img/loading.gif", IDR_UPDATE_CDOS_LOADING, "image/gif"}
};

scoped_refptr<base::RefCountedMemory> GetUpdateHTML() {
  base::DictionaryValue load_time_data;

  load_time_data.SetString("nfsbrowser",
      l10n_util::GetStringUTF16(IDS_CDOSBROWSER));
  load_time_data.SetString("basicbrowser",
      l10n_util::GetStringUTF16(IDS_BASICBROWSER));
  load_time_data.SetString("currentversion",
      l10n_util::GetStringUTF16(IDS_CURRENT_VERSION));
  load_time_data.SetString("checking",
      l10n_util::GetStringUTF16(IDS_CHECK_NEW_VERSION));
  load_time_data.SetString("installing",
      l10n_util::GetStringUTF16(IDS_INSTALL_NOW));
  load_time_data.SetString("downloading",
      l10n_util::GetStringUTF16(IDS_DOWNLOAD_NOW));
  load_time_data.SetString("updatedescription",
      l10n_util::GetStringUTF16(IDS_UPDATE_DESCRIPTION));
  load_time_data.SetString("updatenow",
      l10n_util::GetStringUTF16(IDS_UPDATE));
  load_time_data.SetString("alreadynew",
      l10n_util::GetStringUTF16(IDS_ALREADY_NEW));
  load_time_data.SetString("restartnow",
      l10n_util::GetStringUTF16(IDS_RESTART_NOW));
  load_time_data.SetString("installnow",
      l10n_util::GetStringUTF16(IDS_LINUX_INSTALL_NOW));
  load_time_data.SetString("confirm",
      l10n_util::GetStringUTF16(IDS_CONFORM));
  load_time_data.SetString("refuse",
      l10n_util::GetStringUTF16(IDS_REFUSE));
  load_time_data.SetString("linux_refuse",
      l10n_util::GetStringUTF16(IDS_LINUX_REFUSE));
  load_time_data.SetString("retrynow",
      l10n_util::GetStringUTF16(IDS_RETRYNOW));
  load_time_data.SetString("retry",
      l10n_util::GetStringUTF16(IDS_RETRY));
  load_time_data.SetString("net_error",
      l10n_util::GetStringUTF16(IDS_NET_ERROR));

  // Load the new tab page appropriate for this build.
  base::StringPiece new_tab_html(ResourceBundle::GetSharedInstance().
      GetRawDataResource(IDR_UPDATE_CDOS_MAIN));
  std::string full_html =
      webui::GetI18nTemplateHtml(new_tab_html, &load_time_data);

  return base::RefCountedString::TakeString(&full_html);
}

}  // namespace

UpdateNFSUI::UpdateNFSUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  web_ui->AddMessageHandler(new UpdateHandler(web_ui));

  std::unique_ptr<UpdateHTMLSource> html_source(
      new UpdateHTMLSource(profile->GetOriginalProfile()));

  // content::URLDataSource assumes the ownership of the html_source.
  content::URLDataSource::Add(profile, html_source.release());
}

UpdateNFSUI::~UpdateNFSUI() {

}

///////////////////////////////////////////////////////////////////////////////
//
// UpdateHTMLSource
//
///////////////////////////////////////////////////////////////////////////////
UpdateNFSUI::UpdateHTMLSource::UpdateHTMLSource(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_ );
  for (size_t i = 0; i < arraysize(kResources); ++i) {
    AddResource(kResources[i].filename,
                kResources[i].mime_type,
                kResources[i].identifier);
  }
}

std::string UpdateNFSUI::UpdateHTMLSource::GetSource() const {
  return chrome::kUpdateNfsHost;
}

void UpdateNFSUI::UpdateHTMLSource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::map<std::string, std::pair<std::string, int> >::iterator it =
    resource_map_.find(path);
  if (it != resource_map_.end()) {
    scoped_refptr<base::RefCountedMemory> resource_bytes(
        it->second.second ?
            ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
                it->second.second) :
            new base::RefCountedStaticMemory);
    callback.Run(resource_bytes.get());
    return;
  }

  scoped_refptr<base::RefCountedMemory> html_bytes = GetUpdateHTML();
  callback.Run(html_bytes.get());
}

std::string UpdateNFSUI::UpdateHTMLSource::GetMimeType(const std::string& resource)
    const {
  std::map<std::string, std::pair<std::string, int> >::const_iterator it =
      resource_map_.find(resource);
  if (it != resource_map_.end())
    return it->second.first;
  return "text/html";
}

bool UpdateNFSUI::UpdateHTMLSource::ShouldReplaceExistingSource() const {
  return false;
}

bool UpdateNFSUI::UpdateHTMLSource::ShouldAddContentSecurityPolicy() const {
  return false;
}

void UpdateNFSUI::UpdateHTMLSource::AddResource(const char* resource,
                                             const char* mime_type,
                                             int resource_id) {
  DCHECK(resource);
  DCHECK(mime_type);
  resource_map_[std::string(resource)] =
      std::make_pair(std::string(mime_type), resource_id);
}

UpdateNFSUI::UpdateHTMLSource::~UpdateHTMLSource() {}
