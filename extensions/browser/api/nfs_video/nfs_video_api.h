#include "extensions/common/api/nfs_video.h"
#include "chrome/browser/extensions/chrome_extension_function.h"

namespace extensions {
    class NfsVideoPopupVideoViewFunction : public ChromeUIThreadExtensionFunction {
        public:
            NfsVideoPopupVideoViewFunction();
        protected:
            ~NfsVideoPopupVideoViewFunction() override {}
        private:
            DECLARE_EXTENSION_FUNCTION("nfs.video.popupVideoView", NFS_VIDEO_POPUPVIDEOVIEW);
            ResponseAction Run() final;
    };

    class NfsVideoIsVideoPopupWindowFunction : public ChromeUIThreadExtensionFunction {
        public:
            NfsVideoIsVideoPopupWindowFunction();
        protected:
            ~NfsVideoIsVideoPopupWindowFunction() override {}
        private:
            DECLARE_EXTENSION_FUNCTION("nfs.video.isVideoPopupWindow", NFS_VIDEO_ISVIDEOPOPUPWINDOW);
            ResponseAction Run() final;
    };

    class NfsVideoSetVideoPopupWindowInfoFunction : public ChromeUIThreadExtensionFunction {
        public:
            NfsVideoSetVideoPopupWindowInfoFunction();
        protected:
            ~NfsVideoSetVideoPopupWindowInfoFunction() override {}
        private:
            DECLARE_EXTENSION_FUNCTION("nfs.video.setVideoPopupWindowInfo", NFS_VIDEO_SETVIDEOPOPUPWINDOWINFO);
            ResponseAction Run() final;
    };

    class NfsVideoGetVideoPopupWindowInfoFunction : public ChromeUIThreadExtensionFunction {
        public:
            NfsVideoGetVideoPopupWindowInfoFunction();
        protected:
            ~NfsVideoGetVideoPopupWindowInfoFunction() override {}
        private:
            DECLARE_EXTENSION_FUNCTION("nfs.video.getVideoPopupWindowInfo", NFS_VIDEO_GETVIDEOPOPUPWINDOWINFO);
            ResponseAction Run() final;
    };
}
