chrome.runtime.onMessage.addListener(
	function(request, sender, sendResponse) {
		if (request.type == "NFS_VIDEO_POPUP_WINDOW") {
			chrome.nfs.video.popupVideoView({tabId:sender.tab.id});
		} else if (request.type == "NFS_VIDEO_IS_POPUP_WINDOW") {
			chrome.nfs.video.isVideoPopupWindow({tabId:sender.tab.id}, function IsVideoPopupWindow(o) {sendResponse(o)});
		} else if (request.type == "NFS_VIDEO_SET_VIDEO_POPUP_WINDOW_INFO") {
			chrome.nfs.video.setVideoPopupWindowInfo({tabId:sender.tab.id}, {info:request.videoPopupWindowInfo});
		} else if (request.type == "NFS_VIDEO_GET_VIDEO_POPUP_WINDOW_INFO") {
			chrome.nfs.video.getVideoPopupWindowInfo({tabId:sender.tab.id}, function getVideoPopupWindowInfo(o) {sendResponse(o)});
		}
		return true;
	}
);

chrome.runtime.onMessageExternal.addListener(
	function(request, sender, sendResponse) {
		if (!request.isVideoPopupWindow) {
			chrome.tabs.sendMessage(sender.tab.id, {isVideoPopupWindow: request.isVideoPopupWindow});
		}
	}
);
