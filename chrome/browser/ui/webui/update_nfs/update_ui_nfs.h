// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_UPDATE_NFS_UPDATE_UI_H_
#define CHROME_BROWSER_UI_WEBUI_UPDATE_NFS_UPDATE_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/base/layout.h"
#include "content/public/browser/url_data_source.h"

#include <map>
#include <string>

class Profile;
class UpdateUIHandler;

class UpdateNFSUI : public content::WebUIController {
public:
  explicit UpdateNFSUI(content::WebUI* web_ui);
  ~UpdateNFSUI() override;

private:
  class UpdateHTMLSource : public content::URLDataSource {
   public:
    explicit UpdateHTMLSource(Profile* profile);
    ~UpdateHTMLSource() override;

    // content::URLDataSource implementation.
    std::string GetSource() const override;
    void StartDataRequest(
        const std::string& path,
        const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
        const content::URLDataSource::GotDataCallback& callback) override;
    std::string GetMimeType(const std::string&) const override;
    bool ShouldReplaceExistingSource() const override;
    bool ShouldAddContentSecurityPolicy() const override;

    // Adds |resource| to the source. |resource_id| is resource id or 0,
    // which means return empty data set. |mime_type| is mime type of the
    // resource.
    void AddResource(const char* resource,
                     const char* mime_type,
                     int resource_id);

   private:
    // Pointer back to the original profile.
    Profile* profile_;

    // Maps resource files to mime types an resource ids.
    std::map<std::string, std::pair<std::string, int> > resource_map_;

    DISALLOW_COPY_AND_ASSIGN(UpdateHTMLSource);
  };

  DISALLOW_COPY_AND_ASSIGN(UpdateNFSUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_UPDATE_NFS_UPDATE_UI_H_
