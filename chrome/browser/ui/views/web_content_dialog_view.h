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

#ifndef NFSBROWSER_UI_VIEWS_WEB_CONTENT_DIALOG_VIEW_H_
#define NFSBROWSER_UI_VIEWS_WEB_CONTENT_DIALOG_VIEW_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/views/controls/webview/webview.h"


//  WebContentDialogView is used to show web contents in a dialog.
//  Following code is a example to show/close a WebContentDialogView,assume Browser*
//  browser_ is availabe.
//        nfsbrowser::WebContentDialogView::InitParam params;
//        params.parent = BrowserView::GetBrowserViewForBrowser(browser_)->GetNativeWindow();
//        params.profile = browser_->profile();
//        params.title_align = gfx::ALIGN_CENTER;     //Default title alignment is gfx::ALIGN_LEFT.
//        params.url = GURL(chrome::kChromeUIDownloadsNewURL);  //url intent to load.
//        params.size = gfx::Size(480, 220);   //size of WebContentDialogView.
//        views::widget*  dialog = nfsbrowser::WebContentDialogView::Show(params);
//        ...
//        nfsbrowser::WebContentDialogView::Hide();  //if you want to close it


class Profile;
class GURL;

namespace content {
  class NavigationEntry;
}

namespace nfsbrowser {

class WebContentDialogView : public views::NfsDialogDelegateView {
 public:
  struct InitParam {
    gfx::NativeWindow parent;
    Profile* profile;
    gfx::HorizontalAlignment title_align;
    GURL url;
    gfx::Size size;

    InitParam();
    ~InitParam();
  };

  static void Show(InitParam param);
  static void Hide(content::WebContents* web_contents);

 private:
  explicit WebContentDialogView(InitParam param);
  ~WebContentDialogView() override;

  // views::View:
  gfx::Size GetPreferredSize() const override;
  void Layout() override;

  // views::DialogDelegate:
  int GetDialogButtons() const override;
  base::string16 GetWindowTitle() const override;
  void WindowClosing() override;

  void SetConstrainedDelegate();

 private:
  //gfx::HorizontalAlignment  title_align_;
  views::WebView* web_view_;
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(WebContentDialogView);
};

}	// namespace nfsbrowser
#endif	// NFSBROWSER_UI_VIEWS_WEB_CONTENT_DIALOG_VIEW_H_
