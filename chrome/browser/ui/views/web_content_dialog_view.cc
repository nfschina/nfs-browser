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

#include "chrome/browser/ui/views/web_content_dialog_view.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/widget/widget.h"
#include "chrome/common/url_constants.h"

namespace nfsbrowser {

const char kWebContentDialogViewUserDataKey[] =
    "WebContentDialogViewUserData";

class WebContentDialogViewUserData
    : public base::SupportsUserData::Data {
 public:
  explicit WebContentDialogViewUserData(
      WebContentDialogView* delegate) : delegate_(delegate) {}
  ~WebContentDialogViewUserData() override {}

  WebContentDialogView* delegate() { return delegate_; }

 private:
  WebContentDialogView* delegate_;  // unowned

  DISALLOW_COPY_AND_ASSIGN(WebContentDialogViewUserData);
};

WebContentDialogView* GetConstrainedDelegate(
  content::WebContents* web_contents) {
  WebContentDialogViewUserData* user_data =
      static_cast<WebContentDialogViewUserData*>(
          web_contents->GetUserData(&kWebContentDialogViewUserDataKey));

  return user_data ? user_data->delegate() : NULL;
}

WebContentDialogView::InitParam::InitParam()
  : parent(nullptr),
    profile(nullptr),
    title_align(gfx::ALIGN_LEFT),
    url(GURL()),
    size(gfx::Size(0, 0)) {
}

WebContentDialogView::InitParam::~InitParam() {
}

//static
void WebContentDialogView::Show(InitParam param) {
  WebContentDialogView* web_content_dialog_view =  new WebContentDialogView(param);
  web_content_dialog_view->SetConstrainedDelegate();
  views::Widget* web_content_dialog = constrained_window::CreateNfsModalDialogViews(
                  web_content_dialog_view, param.parent);
  web_content_dialog->Show();
}

//static
void WebContentDialogView::Hide(content::WebContents* web_contents) {
  if (GetConstrainedDelegate(web_contents)) {
    GetConstrainedDelegate(web_contents)->GetWidget()->Close();
  }
}

WebContentDialogView::WebContentDialogView(InitParam param)
  : NfsDialogDelegateView(param.title_align),
    //title_align_(param.title_align),
    size_(param.size) {
  DCHECK(param.size.width() > 0 && param.size.height() > 0);
  DCHECK(param.profile);

  web_view_ = new views::WebView(param.profile);
  web_view_->LoadInitialURL(param.url);
  AddChildView(web_view_);

  web_view_->RequestFocus();

  if (param.url.host() == chrome::kAboutNfsHost) {
    static_cast<views::TopContentsView*>(GetContentsView())->SetBackgroundColor(SK_ColorWHITE);
#if defined(OS_LINUX)
    static_cast<views::TopContentsView*>(GetContentsView())->SetNoDragable();
#else
    static_cast<views::TopContentsView*>(GetContentsView())->SetDragableEverywhere(true);
#endif
  }

  if (param.url.host() == chrome::kChromeUIAccountLoginHost) {
    static_cast<views::TopContentsView*>(GetContentsView())->SetBackgroundColor(SK_ColorWHITE);
    static_cast<views::TopContentsView*>(GetContentsView())->SetTitleVisability(false);
  }

  if (param.url.host() == chrome::kUpdateNfsHost) {
    static SkColor color_update = 0xFF6CA4F2;
    static_cast<views::TopContentsView*>(GetContentsView())->SetBackgroundColor(color_update);
    static_cast<views::TopContentsView*>(GetContentsView())->SetNoDragable();
    static_cast<views::TopContentsView*>(GetContentsView())->SetTitleVisability(false);
  }
  if (param.url.host() == chrome::kAppManagerHost || param.url.host() == chrome::kChromeUIAccountLoginHost) {
#if defined(OS_LINUX)
    static_cast<views::TopContentsView*>(GetContentsView())->SetNoDragable();
#endif
  }
}

gfx::Size WebContentDialogView::GetPreferredSize() const {
  return size_;
}

void WebContentDialogView::Layout() {
   web_view_->SetBounds(0, 0, size_.width(), size_.height());
 }

WebContentDialogView::~WebContentDialogView() {
}

int WebContentDialogView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

base::string16 WebContentDialogView::GetWindowTitle() const {
  content::WebContents* web_contents = web_view_->GetWebContents();

  if (web_contents && web_contents->GetURL().host() == chrome::kAboutNfsHost) {
    return base::string16();
  }

 if (web_contents && web_contents->GetURL().host() == chrome::kAppManagerHost) {
    return l10n_util::GetStringUTF16(IDS_APP_MANAGER_TITLE);
  }

  if (web_contents) {
    return web_contents->GetTitle();
  }
  return base::string16();
}

 void WebContentDialogView::WindowClosing() {
 }

void WebContentDialogView::SetConstrainedDelegate() {
  content::WebContents* web_contents = web_view_->GetWebContents();
  web_contents->SetUserData(&kWebContentDialogViewUserDataKey,
                            new WebContentDialogViewUserData(this));
}

} //namespace nfsbrowser
