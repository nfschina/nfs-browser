/**
 * Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, CAS
 *
 * This software is published by the license of CPU-OS Licence, you can use and
 * distribute this software under this License. See CPU-OS License for more detail.
 *
 * You should have received a copy of CPU-OS License. If not, please contact us
 * by email <support_os@cpu-os.ac.cn>
 *
**/

#ifndef CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_SPEED_UI_H_
#define CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_SPEED_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/base/layout.h"

namespace base {
class RefCountedMemory;
}

namespace content {
class RenderViewHost;
}

class MdDownloadsDOMHandler;

class MdDownloadsSpeedUI : public content::WebUIController {
 public:
  explicit MdDownloadsSpeedUI(content::WebUI* web_ui);

  static base::RefCountedMemory* GetFaviconResourceBytes(
      ui::ScaleFactor scale_factor);

  // content::WebUIController:
  void RenderViewReused(content::RenderViewHost* render_view_host);

 private:
  MdDownloadsDOMHandler* handler_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(MdDownloadsSpeedUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_SPEED_UI_H_
