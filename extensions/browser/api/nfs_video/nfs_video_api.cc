#include "extensions/browser/api/nfs_video/nfs_video_api.h"
#include "base/logging.h"
#include "sys/types.h"
#if !defined(OS_WIN)
#include "unistd.h"
#endif

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace extensions {
    namespace nfs_video = api::nfs_video;

    NfsVideoPopupVideoViewFunction::NfsVideoPopupVideoViewFunction() {}
    ExtensionFunction::ResponseAction NfsVideoPopupVideoViewFunction::Run() {
        std::unique_ptr<nfs_video::PopupVideoView::Params> params(
               nfs_video::PopupVideoView::Params::Create(*args_));
        EXTENSION_FUNCTION_VALIDATE(params.get());
/*
        std::string tmp = "Nfs video PopupVideoView:";
        char buf[100] = {0};
        sprintf(buf, "tabId:%d, video object key:%s", params->options.tab_id, params->options.video_object_key.c_str());
        tmp.append(std::string(buf));
        LOG(INFO) << tmp;
        WriteToConsole(content::CONSOLE_MESSAGE_LEVEL_WARNING, tmp);
*/

        content::WebContents *contents = NULL;
        Browser* browser = NULL;

        if (context_ && params && params->options.tab_id) {
            if (!ExtensionTabUtil::GetTabById(params->options.tab_id,
                    context_,
                    include_incognito(),
                    &browser,
                    NULL,
                    &contents,
                    NULL)) {
                return RespondNow(NoArguments());
            }
            //printf("[%s]extension_id()-->%s\n", __FUNCTION__, extension_id().c_str());
        }

        if (browser) {
            chrome::ExecuteCommand(browser, IDC_DETACH_WEB_VEDIO);
        }

        return RespondNow(NoArguments());
    }

    NfsVideoIsVideoPopupWindowFunction::NfsVideoIsVideoPopupWindowFunction() {}
    ExtensionFunction::ResponseAction NfsVideoIsVideoPopupWindowFunction::Run() {
        std::unique_ptr<nfs_video::IsVideoPopupWindow::Params> params(
               nfs_video::IsVideoPopupWindow::Params::Create(*args_));
        EXTENSION_FUNCTION_VALIDATE(params.get());

        content::WebContents *contents = NULL;
        Browser* browser = NULL;

        if (context_ && params && params->options.tab_id) {
            if (!ExtensionTabUtil::GetTabById(params->options.tab_id,
                    context_,
                    include_incognito(),
                    &browser,
                    NULL,
                    &contents,
                    NULL)) {
                return RespondNow(NoArguments());
            }
            //printf("[%s]extension_id()-->%s\n", __FUNCTION__, extension_id().c_str());
        }

        if (browser && contents) {
            bool isVideoPopupWindow = contents->GetMainFrame()->getVideoPopupWindowStatus();
            std::unique_ptr<base::ListValue> output =
            nfs_video::IsVideoPopupWindow::Results::Create(isVideoPopupWindow);
            return RespondNow(ArgumentList(std::move(output)));
        }

        return RespondNow(NoArguments());
    }

    NfsVideoSetVideoPopupWindowInfoFunction::NfsVideoSetVideoPopupWindowInfoFunction() {}
    ExtensionFunction::ResponseAction NfsVideoSetVideoPopupWindowInfoFunction::Run() {
        std::unique_ptr<nfs_video::SetVideoPopupWindowInfo::Params> params(
               nfs_video::SetVideoPopupWindowInfo::Params::Create(*args_));
        EXTENSION_FUNCTION_VALIDATE(params.get());

        content::WebContents *contents = NULL;
        Browser* browser = NULL;

        if (context_ && params && params->options.tab_id) {
            if (!ExtensionTabUtil::GetTabById(params->options.tab_id,
                    context_,
                    include_incognito(),
                    &browser,
                    NULL,
                    &contents,
                    NULL)) {
                return RespondNow(NoArguments());
            }
            //printf("[%s]extension_id()-->%s\n", __FUNCTION__, extension_id().c_str());
        }

        if (browser && contents) {
            contents->GetMainFrame()->setVideoPopupWindowInfo(params->video_popup_window_info.info);
            //printf("[SET][%s][%s]\n", __FUNCTION__, params->video_popup_window_info.info.c_str());
        }

        return RespondNow(NoArguments());
    }

    NfsVideoGetVideoPopupWindowInfoFunction::NfsVideoGetVideoPopupWindowInfoFunction() {}
    ExtensionFunction::ResponseAction NfsVideoGetVideoPopupWindowInfoFunction::Run() {
        std::unique_ptr<nfs_video::GetVideoPopupWindowInfo::Params> params(
               nfs_video::GetVideoPopupWindowInfo::Params::Create(*args_));
        EXTENSION_FUNCTION_VALIDATE(params.get());

        content::WebContents *contents = NULL;
        Browser* browser = NULL;

        if (context_ && params && params->options.tab_id) {
            if (!ExtensionTabUtil::GetTabById(params->options.tab_id,
                    context_,
                    include_incognito(),
                    &browser,
                    NULL,
                    &contents,
                    NULL)) {
                return RespondNow(NoArguments());
            }
            //printf("[%s]extension_id()-->%s\n", __FUNCTION__, extension_id().c_str());
        }

        if (browser && contents) {
            std::string info = contents->GetMainFrame()->getVideoPopupWindowInfo();
            //printf("[GET][%s][%s]\n", __FUNCTION__, info.c_str());
            nfs_video::VideoPopupWindowInfo videoPopupWindowInfo;
            videoPopupWindowInfo.info = info;
            std::unique_ptr<base::ListValue> output =
            nfs_video::GetVideoPopupWindowInfo::Results::Create(videoPopupWindowInfo);
            return RespondNow(ArgumentList(std::move(output)));
        }

        return RespondNow(NoArguments());
    }

}//end extensions
