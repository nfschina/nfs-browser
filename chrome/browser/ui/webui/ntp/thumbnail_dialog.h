#ifndef CHROME_BROWSER_UI_WEBUI_NTP_DIALOG_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_DIALOG_H_

#include "base/macros.h"
#include "base/values.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"
#include "extensions/browser/api/web_contents_capture_client.h"
#include "extensions/common/api/extension_types.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

class Profile;
class SkBitmap;

namespace views {
class Widget;
}

class ThumbnailDialog : public extensions::WebContentsCaptureClient
                      , public content::WebContentsObserver
                      , public ui::WebDialogDelegate {
public:
  enum FetchThumbnailResult {
    FETCH_THUMBNAIL_SUCCESS,
    FETCH_THUMBNAIL_TIME_OUT,
    FETCH_THUMBNAIL_OTHER_REASON
  };

  typedef base::Callback<void(const FetchThumbnailResult,
                              const int, const GURL&)>
      FetchThumbnailCallback;

  explicit ThumbnailDialog(Profile* profile, int index, GURL& url);
  ~ThumbnailDialog() override;

  static gfx::NativeWindow Show(Profile* profile, int index, GURL& url);
  static views::Widget* CreateWebWidget(Profile* profile,
                                        int index,
                                        GURL& url,
                                        FetchThumbnailCallback callback);

  views::Widget* widget() { return widget_; }

protected:
  // ui::WebDialogDelegate:
  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnLoadingStateChanged(content::WebContents* source) override;
  void OnDialogShown(content::WebUI* webui,
                     content::RenderViewHost* render_view_host) override;
  // NOTE: This function deletes this object at the end.
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;
  void OnWebContentCreated(content::WebContents* source) override;

private:
  // extensions::WebContentsCaptureClient:
  bool IsScreenshotEnabled() override;
  bool ClientAllowsTransparency() override;
  bool CaptureAsync(content::WebContents* web_contents,
                    const extensions::api::extension_types::ImageDetails* image_detail,
                    const content::ReadbackRequestCallback callback);
  bool EncodeBitmap(const SkBitmap& bitmap, std::string* base64_result);
  void OnCaptureSuccess(const SkBitmap& bitmap) override;
  void OnCaptureFailure(FailureReason reason) override;

  // content::WebContentsObserver ----------------------------------------------
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description,
                   bool was_ignored_by_handler) override;

  void StartCapture();
  void GetWidgetAndCallback(views::Widget* widget,
                            FetchThumbnailCallback callback);
  void CloseThumbnailDialog();
  void OnTimeOut();

  FetchThumbnailCallback callback_;

  bool captured_;
  int index_;
  GURL url_;
  base::string16 title_;

  base::Time begin_time_;

  Profile* profile_;

  views::Widget* widget_;

  content::WebContents* contents_;

  // The format (JPEG vs PNG) of the resulting image.
  extensions::api::extension_types::ImageFormat image_format_;

  // Quality setting to use when encoding jpegs.
  int image_quality_;

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<ThumbnailDialog> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailDialog);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_DIALOG_H_
