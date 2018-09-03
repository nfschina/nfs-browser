// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "content/public/common/url_constants.h"

namespace content {

// Before adding new chrome schemes please check with security@chromium.org.
// There are security implications associated with introducing new schemes.
const char kChromeDevToolsScheme[] = "nfsbrowser-devtools";
const char kChromeUIScheme[] = "nfsbrowser";
const char kChromeExtentionScheme[] = "nfsbrowser-extension";
const char kGuestScheme[] = "nfsbrowser-guest";
const char kViewSourceScheme[] = "view-source";
#if defined(OS_CHROMEOS)
const char kExternalFileScheme[] = "externalfile";
#endif

const char kAboutSrcDocURL[] = "about:srcdoc";

const char kChromeUIAppCacheInternalsHost[] = "appcache-internals";
const char kChromeUIIndexedDBInternalsHost[] = "indexeddb-internals";
const char kChromeUIAccessibilityHost[] = "accessibility";
const char kChromeUIBlobInternalsHost[] = "blob-internals";
const char kChromeUIBrowserCrashHost[] = "inducebrowsercrashforrealz";
const char kChromeUIGpuHost[] = "gpu";
const char kChromeUIHistogramHost[] = "histograms";
const char kChromeUIHistoryHost[] = "history";
const char kChromeUIMediaInternalsHost[] = "media-internals";
const char kChromeUIMemoryExhaustHost[] = "memory-exhaust";
const char kChromeUINetworkViewCacheHost[] = "view-http-cache";
const char kChromeUINetworkErrorHost[] = "network-error";
const char kChromeUINetworkErrorsListingHost[] = "network-errors";
const char kChromeUIResourcesHost[] = "resources";
const char kChromeUIServiceWorkerInternalsHost[] = "serviceworker-internals";
const char kChromeUITracingHost[] = "tracing";
const char kChromeUIUberHost[] = "chrome";
const char kChromeUIWebRTCInternalsHost[] = "webrtc-internals";

const char kChromeUIBadCastCrashURL[] = "nfsbrowser://badcastcrash";
const char kChromeUIBrowserCrashURL[] = "nfsbrowser://inducebrowsercrashforrealz";
const char kChromeUIBrowserUIHang[] = "nfsbrowser://uithreadhang";
const char kChromeUICrashURL[] = "nfsbrowser://crash";
const char kChromeUIDelayedBrowserUIHang[] = "nfsbrowser://delayeduithreadhang";
const char kChromeUIDumpURL[] = "nfsbrowser://crashdump";
const char kChromeUIGpuCleanURL[] = "nfsbrowser://gpuclean";
const char kChromeUIGpuCrashURL[] = "nfsbrowser://gpucrash";
const char kChromeUIGpuHangURL[] = "nfsbrowser://gpuhang";
const char kChromeUIHangURL[] = "nfsbrowser://hang";
const char kChromeUIKillURL[] = "nfsbrowser://kill";
const char kChromeUIMemoryExhaustURL[] = "nfsbrowser://memory-exhaust";
const char kChromeUINetworkErrorURL[] = "nfsbrowser://network-error";
const char kChromeUINetworkErrorsListingURL[] = "nfsbrowser://network-errors";
const char kChromeUIPpapiFlashCrashURL[] = "nfsbrowser://ppapiflashcrash";
const char kChromeUIPpapiFlashHangURL[] = "nfsbrowser://ppapiflashhang";

// This error URL is loaded in normal web renderer processes, so it should not
// have a nfsbrowser:// scheme that might let it be confused with a WebUI page.
const char kUnreachableWebDataURL[] = "data:text/html,nfsbrowserwebdata";

const char kChromeUINetworkViewCacheURL[] = "nfsbrowser://view-http-cache/";
const char kChromeUIResourcesURL[] = "nfsbrowser://resources/";
const char kChromeUIShorthangURL[] = "nfsbrowser://shorthang";

const char kHttpSuboriginScheme[] = "http-so";
const char kHttpsSuboriginScheme[] = "https-so";

}  // namespace content
