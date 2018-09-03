#include "chrome/browser/ui/webui/ntp/thumbnail_dialog.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/thumbnails/thumbnailing_algorithm.h"
#include "chrome/browser/thumbnails/thumbnail_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/ntp/new_tab_handler.h"
#include "chrome/common/url_constants.h"
#include "components/history/core/browser/top_sites.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "extensions/common/constants.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/scrollbar_size.h"
#include "ui/views/widget/widget.h"

using content::BrowserThread;
using content::ReadbackRequestCallback;
using content::RenderFrameHost;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::RenderWidgetHostView;
using content::WebContents;

using namespace extensions;
using views::View;

namespace {

const int kDefaultWidth = 1000;
const int kDefaultHeight = 800;

}  // namespace

// static
gfx::NativeWindow ThumbnailDialog::Show(Profile* profile,
                                        int index,
                                        GURL& url) {
  DCHECK(profile);

  return chrome::ShowWebDialog(NULL, profile,
                               new ThumbnailDialog(profile, index, url));
}

views::Widget* ThumbnailDialog::CreateWebWidget(
    Profile* profile,
    int index,
    GURL& url,
    FetchThumbnailCallback callback) {
  DCHECK(profile);

  ThumbnailDialog* thumbnail_dialog(
      new ThumbnailDialog(profile, index, url));

  views::Widget* widget = chrome::CreateWebWidget(NULL,
                                                  profile,
                                                  thumbnail_dialog);

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&ThumbnailDialog::GetWidgetAndCallback,
                                     base::Unretained(thumbnail_dialog),
                                     widget, callback));

  return widget;

  // return chrome::CreateWebWidget(NULL, profile,
  //                                new ThumbnailDialog(profile, index, url));
}

ThumbnailDialog::ThumbnailDialog(Profile* profile, int index, GURL& url)
    : captured_(false)
    , index_(index)
    , url_(url)
    , profile_(profile)
    , widget_(NULL)
    , contents_(NULL)
    , weak_ptr_factory_(this) {
  begin_time_ = base::Time::Now();

  // 60s之内无法获取截图认为失败
  BrowserThread::PostDelayedTask(BrowserThread::UI, FROM_HERE,
                                 base::Bind(&ThumbnailDialog::OnTimeOut,
                                            weak_ptr_factory_.GetWeakPtr()),
                                 base::TimeDelta::FromSeconds(30));
}

ThumbnailDialog::~ThumbnailDialog() {

}

void ThumbnailDialog::StartCapture() {
  // The default format and quality setting used when encoding jpegs.
  const api::extension_types::ImageFormat kDefaultFormat =
      api::extension_types::IMAGE_FORMAT_JPEG;
  const int kDefaultQuality = 90;

  image_format_ = kDefaultFormat;
  image_quality_ = kDefaultQuality;

  CaptureAsync(contents_, NULL,
               base::Bind(&ThumbnailDialog::CopyFromBackingStoreComplete,
                          base::Unretained(this)));
}

void ThumbnailDialog::GetWidgetAndCallback(views::Widget* widget,
                                           FetchThumbnailCallback callback) {
  DCHECK(widget);
  widget_   = widget;
  callback_ = callback;
}

void ThumbnailDialog::CloseThumbnailDialog() {
  if (widget_) {
    widget_->Close();
  }
}

void ThumbnailDialog::OnTimeOut() {
  if (captured_)
    return ;

  callback_.Run(FETCH_THUMBNAIL_TIME_OUT,
                index_, url_);
  CloseThumbnailDialog();
}

ui::ModalType ThumbnailDialog::GetDialogModalType() const {
  return ui::MODAL_TYPE_NONE;
}

base::string16 ThumbnailDialog::GetDialogTitle() const {
  return base::ASCIIToUTF16("thumbnail");
}

GURL ThumbnailDialog::GetDialogContentURL() const {
  return url_;
}

void ThumbnailDialog::GetWebUIMessageHandlers(
    std::vector<content::WebUIMessageHandler*>* handler) const {
}

void ThumbnailDialog::GetDialogSize(gfx::Size* size) const {
  size->SetSize(kDefaultWidth, kDefaultHeight);
}

std::string ThumbnailDialog::GetDialogArgs() const {
  return std::string();
}

void ThumbnailDialog::OnLoadingStateChanged(WebContents* source) {
}

void ThumbnailDialog::OnDialogShown(
    content::WebUI* webui,
    content::RenderViewHost* render_view_host) {
}

void ThumbnailDialog::OnDialogClosed(const std::string& json_retval) {
  delete this;
}

void ThumbnailDialog::OnCloseContents(
    content::WebContents* source, bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool ThumbnailDialog::ShouldShowDialogTitle() const {
  return false;
}

void ThumbnailDialog::OnWebContentCreated(WebContents* source) {
  DCHECK(source);

  title_ = source->GetTitle();
  WebContentsObserver::Observe(source);

  source->SetAudioMuted(true);
  contents_ = source;
}

/*********************************WebContentsCaptureClient******************************************/
bool ThumbnailDialog::IsScreenshotEnabled() {
  return true;
}

bool ThumbnailDialog::ClientAllowsTransparency() {
  return false;
}

bool ThumbnailDialog::CaptureAsync(
    content::WebContents* web_contents,
    const api::extension_types::ImageDetails* image_detail,
    const content::ReadbackRequestCallback callback) {
  if (!IsScreenshotEnabled())
    return false;

  const api::extension_types::ImageFormat kDefaultFormat =
      api::extension_types::IMAGE_FORMAT_PNG;
  const int kDefaultQuality = 90;

  image_format_ = kDefaultFormat;
  image_quality_ = kDefaultQuality;

  // TODO(miu): Account for fullscreen render widget?  http://crbug.com/419878
  RenderWidgetHostView* const view = web_contents->GetRenderWidgetHostView();
  RenderWidgetHost* const host = view ? view->GetRenderWidgetHost() : nullptr;
  if (!view || !host) {
    OnCaptureFailure(FAILURE_REASON_VIEW_INVISIBLE);
    return false;
  }

  gfx::Rect copy_rect = gfx::Rect(view->GetViewBounds().size());
  // Clip the pixels that will commonly hold a scrollbar, which looks bad in
  // thumbnails.
  int scrollbar_size = gfx::scrollbar_size();
  copy_rect.Inset(0, 0, scrollbar_size, scrollbar_size);

  if (copy_rect.IsEmpty()) {
    return false;
  }

  scoped_refptr<thumbnails::ThumbnailService> thumbnail_service =
      ThumbnailServiceFactory::GetForProfile(profile_);
  if (thumbnail_service.get() == NULL) {
    return false;
  }

  scoped_refptr<thumbnails::ThumbnailingAlgorithm> algorithm(
      thumbnail_service->GetThumbnailingAlgorithm());
  if (algorithm.get() == NULL) {
    return false;
  }

  ui::ScaleFactor scale_factor =
      ui::GetSupportedScaleFactor(
          ui::GetScaleFactorForNativeView(view->GetNativeView()));
  gfx::Size copy_size;
  algorithm->GetCanvasCopyInfo(copy_rect.size(), scale_factor, &copy_rect, &copy_size);

  host->CopyFromBackingStore(copy_rect, copy_size, callback, kN32_SkColorType);

  return true;
}

bool ThumbnailDialog::EncodeBitmap(const SkBitmap& bitmap,
                                   std::string* base64_result) {
  DCHECK(base64_result);
  std::vector<unsigned char> data;
  SkAutoLockPixels screen_capture_lock(bitmap);
  const bool should_discard_alpha = !ClientAllowsTransparency();
  bool encoded = false;
  std::string mime_type;
  switch (image_format_) {
    case api::extension_types::IMAGE_FORMAT_JPEG:
      encoded = gfx::JPEGCodec::Encode(
          reinterpret_cast<unsigned char*>(bitmap.getAddr32(0, 0)),
          gfx::JPEGCodec::FORMAT_SkBitmap, bitmap.width(), bitmap.height(),
          static_cast<int>(bitmap.rowBytes()), image_quality_, &data);
      mime_type = extensions::kMimeTypeJpeg;
      break;
    case api::extension_types::IMAGE_FORMAT_PNG:
      encoded = gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, should_discard_alpha,
                                                  &data);
      mime_type = extensions::kMimeTypePng;
      break;
    default:
      NOTREACHED() << "Invalid image format.";
  }

  if (!encoded)
    return false;

  base::StringPiece stream_as_string(reinterpret_cast<const char*>(data.data()),
                                     data.size());

  base::Base64Encode(stream_as_string, base64_result);
  base64_result->insert(
      0, base::StringPrintf("data:%s;base64,", mime_type.c_str()));

  return true;
}

void ThumbnailDialog::OnCaptureSuccess(const SkBitmap& bitmap) {
  std::string base64_result;
  if (!EncodeBitmap(bitmap, &base64_result)) {
    OnCaptureFailure(FAILURE_REASON_ENCODING_FAILED);
    return ;
  }

  scoped_refptr<history::TopSites> top_sites = TopSitesFactory::GetForProfile(profile_);
  DCHECK(top_sites.get());

  if (index_ != -1) {

  }

  gfx::Image image = gfx::Image::CreateFrom1xBitmap(bitmap);
  top_sites->SetPageCapture(url_, title_, index_, image);

  captured_ = true;
  callback_.Run(FETCH_THUMBNAIL_SUCCESS,
                index_, url_);

  CloseThumbnailDialog();
}

void ThumbnailDialog::OnCaptureFailure(FailureReason reason) {
  callback_.Run(FETCH_THUMBNAIL_OTHER_REASON,
              index_, url_);
  CloseThumbnailDialog();
}

// content::WebContentsObserver ------------------------------------------------

void ThumbnailDialog::DidFinishLoad(RenderFrameHost* render_frame_host,
                                    const GURL& validated_url) {
  BrowserThread::PostDelayedTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ThumbnailDialog::StartCapture,
                 weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(3));
}

void ThumbnailDialog::DidFailLoad(content::RenderFrameHost* render_frame_host,
                                  const GURL& validated_url,
                                  int error_code,
                                  const base::string16& error_description,
                                  bool was_ignored_by_handler) {
  LOG(ERROR) << "DidFailLoad";
}
