// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/ui/webui/nfs_webpush/nfs_webpush.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/browser_resources.h"
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
    {"notification.js", IDR_WEBPUSH_CDOS_JS, "application/javascript"},
    {"jquery.js", IDR_WEBPUSH_CDOS_JQUERY, "application/javascript"}
};

scoped_refptr<base::RefCountedMemory> GetWebPushHTML() {
  base::DictionaryValue load_time_data;

  // Load the new tab page appropriate for this build.
  base::StringPiece webpush_html(ResourceBundle::GetSharedInstance().
      GetRawDataResource(IDR_WEBPUSH_CDOS_MAIN));
  std::string full_html =
      webui::GetI18nTemplateHtml(webpush_html, &load_time_data);

  return base::RefCountedString::TakeString(&full_html);
}

}  // namespace

NFSWebPushUI::NFSWebPushUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  std::unique_ptr<NFSWebPushHTMLSource> html_source(
      new NFSWebPushHTMLSource(profile->GetOriginalProfile()));

  // content::URLDataSource assumes the ownership of the html_source.
  content::URLDataSource::Add(profile, html_source.release());
}

NFSWebPushUI::~NFSWebPushUI() {

}

///////////////////////////////////////////////////////////////////////////////
//
// UpdateHTMLSource
//
///////////////////////////////////////////////////////////////////////////////
NFSWebPushUI::NFSWebPushHTMLSource::NFSWebPushHTMLSource(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_ );
  for (size_t i = 0; i < arraysize(kResources); ++i) {
    AddResource(kResources[i].filename,
                kResources[i].mime_type,
                kResources[i].identifier);
  }
}

std::string NFSWebPushUI::NFSWebPushHTMLSource::GetSource() const {
  return chrome::kWebPushNfsHost;
}

void NFSWebPushUI::NFSWebPushHTMLSource::StartDataRequest(
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

  scoped_refptr<base::RefCountedMemory> html_bytes = GetWebPushHTML();
  callback.Run(html_bytes.get());
}

std::string NFSWebPushUI::NFSWebPushHTMLSource::GetMimeType(const std::string& resource)
    const {
  std::map<std::string, std::pair<std::string, int> >::const_iterator it =
      resource_map_.find(resource);
  if (it != resource_map_.end())
    return it->second.first;
  return "text/html";
}

bool NFSWebPushUI::NFSWebPushHTMLSource::ShouldReplaceExistingSource() const {
  return false;
}

bool NFSWebPushUI::NFSWebPushHTMLSource::ShouldAddContentSecurityPolicy() const {
  return false;
}

void NFSWebPushUI::NFSWebPushHTMLSource::AddResource(const char* resource,
                                             const char* mime_type,
                                             int resource_id) {
  DCHECK(resource);
  DCHECK(mime_type);
  resource_map_[std::string(resource)] =
      std::make_pair(std::string(mime_type), resource_id);
}

NFSWebPushUI::NFSWebPushHTMLSource::~NFSWebPushHTMLSource() {}
