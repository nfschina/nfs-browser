// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_FACTORY_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_FACTORY_H_

#include <memory>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace crypto {
class SecureHash;
}

namespace net {
class NetLogWithSource;
class AuthCredentials;
}

namespace content {

class ByteStreamReader;
class DownloadDestinationObserver;
class DownloadFile;
class DownloadManager;
class DownloadRequestHandleInterface;
struct DownloadSaveInfo;

class CONTENT_EXPORT DownloadFileFactory {
 public:
  virtual ~DownloadFileFactory();

  virtual DownloadFile* CreateFile(
      std::unique_ptr<DownloadSaveInfo> save_info,
      const base::FilePath& default_downloads_directory,
      std::unique_ptr<ByteStreamReader> byte_stream,
      const net::NetLogWithSource& net_log,
      base::WeakPtr<DownloadDestinationObserver> observer);

  virtual DownloadFile* CreateFileSpeed(
      uint32_t download_id,
      int64_t bytes_total,
      int64_t bytes_received,
      std::string cookies,
      GURL url_chain,
      GURL referrer_url,
      std::string method,
      std::string pack_url,
      std::unique_ptr<DownloadSaveInfo> save_info,
      net::AuthCredentials auth_credential,
      const base::FilePath& default_downloads_directory,
      std::unique_ptr<ByteStreamReader> byte_stream,
      const net::NetLogWithSource& bound_net_log,
      base::WeakPtr<DownloadDestinationObserver> observer,
      bool task_status,
      bool is_breakpoint,
      DownloadManager* download_manager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_FACTORY_H_
