// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_file_factory.h"

#include <utility>

#include "content/browser/download/download_file_impl.h"
#include "content/browser/download/download_file_impl_speed.h"

namespace content {

DownloadFileFactory::~DownloadFileFactory() {}

DownloadFile* DownloadFileFactory::CreateFile(
    std::unique_ptr<DownloadSaveInfo> save_info,
    const base::FilePath& default_downloads_directory,
    std::unique_ptr<ByteStreamReader> byte_stream,
    const net::NetLogWithSource& net_log,
    base::WeakPtr<DownloadDestinationObserver> observer) {
  return new DownloadFileImpl(std::move(save_info),
                              default_downloads_directory,
                              std::move(byte_stream),
                              net_log,
                              observer);
}

DownloadFile* DownloadFileFactory::CreateFileSpeed(
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
    DownloadManager* download_manager) {
  return new DownloadFileImplSpeed(download_id,
                                   bytes_total,
                                   bytes_received,
                                   cookies,
                                   url_chain,
                                   referrer_url,
                                   method,
                                   pack_url,
                                   std::move(save_info),
                                   auth_credential,
                                   default_downloads_directory,
                                   std::move(byte_stream),
                                   bound_net_log,
                                   observer,
                                   task_status,
                                   is_breakpoint,
                                   download_manager);
}

}  // namespace content
