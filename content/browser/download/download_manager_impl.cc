// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, Chinese Academy of Sciences.

#include "content/browser/download/download_manager_impl.h"

#include <iterator>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/debug/alias.h"
#include "base/files/file_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/supports_user_data.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_restrictions.h"
#include "base/synchronization/waitable_event.h"
#include "build/build_config.h"
#include "content/browser/byte_stream.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_file_factory.h"
#include "content/browser/download/download_file_impl_speed.h"
#include "content/browser/download/download_item_factory.h"
#include "content/browser/download/download_item_impl.h"
#include "content/browser/download/download_stats.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/referrer.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/filename_util.h"
#include "net/base/load_flags.h"
#include "net/base/request_priority.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/cookies/cookie_store.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_request_context.h"
#include "storage/browser/blob/blob_url_request_job_factory.h"
#include "url/origin.h"

#include "base/command_line.h"

 #if defined(OS_POSIX)
 #define STRING_LITERAL(x) x
 typedef std::string string16;
 #elif defined(OS_WIN)
 #define STRING_LITERAL(x) L ## x
 typedef std::wstring string16;
 #endif

namespace {
// huk
void WriteInfoIntoTmpFile(std::string file_full_path) {
  base::FilePath target_path = base::FilePath(STRING_LITERAL("/tmp/nfs_download.tmp"));

  if (PathExists(target_path))  {
    bool ret = base::AppendToFile(target_path, file_full_path.c_str(), file_full_path.size());
    DCHECK(ret);
  } else {
    base::WriteFile(target_path, file_full_path.c_str(), file_full_path.size());
  }
}
}

namespace content {
namespace {

std::unique_ptr<UrlDownloader, BrowserThread::DeleteOnIOThread> BeginDownload(
    std::unique_ptr<DownloadUrlParameters> params,
    content::ResourceContext* resource_context,
    uint32_t download_id,
    base::WeakPtr<DownloadManagerImpl> download_manager) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<net::URLRequest> url_request =
      DownloadRequestCore::CreateRequestOnIOThread(download_id, params.get());
  std::unique_ptr<storage::BlobDataHandle> blob_data_handle =
      params->GetBlobDataHandle();
  if (blob_data_handle) {
    storage::BlobProtocolHandler::SetRequestedBlobDataHandle(
        url_request.get(), std::move(blob_data_handle));
  }

  // If there's a valid renderer process associated with the request, then the
  // request should be driven by the ResourceLoader. Pass it over to the
  // ResourceDispatcherHostImpl which will in turn pass it along to the
  // ResourceLoader.
  if (params->render_process_host_id() >= 0) {
    DownloadInterruptReason reason = DownloadManagerImpl::BeginDownloadRequest(
        std::move(url_request), params->referrer(), resource_context,
        params->content_initiated(), params->render_process_host_id(),
        params->render_view_host_routing_id(),
        params->render_frame_host_routing_id(),
        params->do_not_prompt_for_login());

    // If the download was accepted, the DownloadResourceHandler is now
    // responsible for driving the request to completion.
    if (reason == DOWNLOAD_INTERRUPT_REASON_NONE)
      return nullptr;

    // Otherwise, create an interrupted download.
    std::unique_ptr<DownloadCreateInfo> failed_created_info(
        new DownloadCreateInfo(base::Time::Now(), net::NetLogWithSource(),
                               base::WrapUnique(new DownloadSaveInfo)));
    failed_created_info->url_chain.push_back(params->url());
    failed_created_info->result = reason;
    std::unique_ptr<ByteStreamReader> empty_byte_stream;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&DownloadManager::StartDownload, download_manager,
                   base::Passed(&failed_created_info),
                   base::Passed(&empty_byte_stream), params->callback()));
    return nullptr;
  }

  return std::unique_ptr<UrlDownloader, BrowserThread::DeleteOnIOThread>(
      UrlDownloader::BeginDownload(download_manager, std::move(url_request),
                                   params->referrer())
          .release());
}

bool PathExistsInDownloads(base::FilePath target_path,
    content::DownloadManager::DownloadVector* downloads) {
  DCHECK(downloads);

  // check if the target path in the downloads
  DownloadManager::DownloadVector::iterator it;
  for (it = downloads->begin(); it != downloads->end(); ++it) {
    base::FilePath item_path = (*it)->GetTargetFilePath();
    if (item_path == target_path) {
      return true;
    }
  }

  return false;
}

base::string16 GetSuggestedFileName(GURL url,
    std::string mime_type,
    std::string content_disposition,
    std::string referrer_charset,
    std::string default_filename,
    base::FilePath default_directory,
    content::DownloadManager::DownloadVector* downloads) {
  base::FilePath generated_filename = net::GenerateFileName(url,
      content_disposition,
      referrer_charset,
      "",
      mime_type,
      default_filename);

  base::FilePath base_name = generated_filename.BaseName().RemoveExtension();
  string16 extension = generated_filename.Extension();
  if (extension.empty()) {
    if (url.SchemeIsData()) {
      if (mime_type == "application/octet-streampeg")  //.jpe
          extension = STRING_LITERAL("jpg");
      else   //.gif
          extension = STRING_LITERAL("gif");
     }
  }

  base::FilePath target_path = default_directory.Append(base_name);
  target_path = target_path.AddExtension(extension);

  int index = 1;
  bool previous_disallowed = base::ThreadRestrictions::SetIOAllowed(true);

  while (base::PathExists(target_path) || PathExistsInDownloads(target_path, downloads)) {
    #if defined(OS_POSIX)
    string16 serial_code = base::IntToString(index++);
    #elif defined(OS_WIN)
    string16 serial_code = base::IntToString16(index++);
    #endif
    base::FilePath new_base_name = base::FilePath(base_name.value()
      + STRING_LITERAL("(") + serial_code + STRING_LITERAL(")"));
    target_path = default_directory.Append(new_base_name).AddExtension(extension);
  }

  base::ThreadRestrictions::SetIOAllowed(previous_disallowed);
  string16 suggested_filename = target_path.BaseName().value();
  #if defined(OS_POSIX)
  return base::UTF8ToUTF16(suggested_filename);
  #elif defined(OS_WIN)
  return suggested_filename;
  #endif
}

class DownloadItemFactoryImpl : public DownloadItemFactory {
 public:
  DownloadItemFactoryImpl() {}
  ~DownloadItemFactoryImpl() override {}

  DownloadItemImpl* CreatePersistedItem(
      DownloadItemImplDelegate* delegate,
      const std::string& guid,
      uint32_t download_id,
      const base::FilePath& current_path,
      const base::FilePath& target_path,
      const std::vector<GURL>& url_chain,
      const GURL& referrer_url,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_refererr_url,
      const std::string& method,
      const std::string& pack_url,
      const std::string& mime_type,
      const std::string& original_mime_type,
      const base::Time& start_time,
      const base::Time& end_time,
      const std::string& etag,
      const std::string& last_modified,
      const std::string& bt_real_name,
      int64_t received_bytes,
      int64_t total_bytes,
      const std::string& hash,
      DownloadItem::DownloadState state,
      DownloadDangerType danger_type,
      DownloadInterruptReason interrupt_reason,
      bool opened,
      const net::NetLogWithSource& net_log) override {
    return new DownloadItemImpl(
        delegate, guid, download_id, current_path, target_path, url_chain,
        referrer_url, site_url, tab_url, tab_refererr_url, method, pack_url, mime_type,
        original_mime_type, start_time, end_time, etag, last_modified, bt_real_name,
        received_bytes, total_bytes, hash, state, danger_type, interrupt_reason,
        opened, net_log);
  }

  DownloadItemImpl* CreateActiveItem(
      DownloadItemImplDelegate* delegate,
      uint32_t download_id,
      const DownloadCreateInfo& info,
      const net::NetLogWithSource& net_log) override {
    return new DownloadItemImpl(delegate, download_id, info, net_log);
  }

  DownloadItemImpl* CreateSavePageItem(
      DownloadItemImplDelegate* delegate,
      uint32_t download_id,
      const base::FilePath& path,
      const GURL& url,
      const std::string& mime_type,
      std::unique_ptr<DownloadRequestHandleInterface> request_handle,
      const net::NetLogWithSource& net_log) override {
    return new DownloadItemImpl(delegate, download_id, path, url, mime_type,
                                std::move(request_handle), net_log);
  }
};

}  // namespace

DownloadManagerImpl::DownloadManagerImpl(
    net::NetLog* net_log,
    BrowserContext* browser_context)
    : item_factory_(new DownloadItemFactoryImpl()),
      file_factory_(new DownloadFileFactory()),
      history_size_(0),
      shutdown_needed_(true),
      browser_context_(browser_context),
      delegate_(NULL),
      net_log_(net_log),
      weak_factory_(this) {
  DCHECK(browser_context);
}

DownloadManagerImpl::~DownloadManagerImpl() {
  DCHECK(!shutdown_needed_);
}

void DownloadManagerImpl::Init() {
//#if defined(OS_LINUX)
  base::FilePath user_data_path;
  if (delegate_) {
    user_data_path = delegate_->GetUserDataPath();
  } else {
    user_data_path = GetDefaultDownloadPath();
  }

  DownloadFileImplSpeed::ApxDownloadInit(user_data_path);
//#endif
}

DownloadItemImpl* DownloadManagerImpl::CreateActiveItem(
    uint32_t id,
    const DownloadCreateInfo& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!base::ContainsKey(downloads_, id));
  net::NetLogWithSource net_log =
      net::NetLogWithSource::Make(net_log_, net::NetLogSourceType::DOWNLOAD);

  DownloadItemImpl* download =
      item_factory_->CreateActiveItem(this, id, info, net_log);


  downloads_[id] = download;
  downloads_by_guid_[download->GetGuid()] = download;

  //huk
  if (ShouldUseSaveAsDownloadMode(download->GetURL(), true))  {
    download->SetSaveAsDownloadMode(true);
  }

  return download;
}

void DownloadManagerImpl::GetNextId(const DownloadIdCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (delegate_) {
    delegate_->GetNextId(callback);
    return;
  }
  static uint32_t next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

void DownloadManagerImpl::DetermineDownloadTarget(
    DownloadItemImpl* item, const DownloadTargetCallback& callback) {
  // Note that this next call relies on
  // DownloadItemImplDelegate::DownloadTargetCallback and
  // DownloadManagerDelegate::DownloadTargetCallback having the same
  // type.  If the types ever diverge, gasket code will need to
  // be written here.
  if (!delegate_ || !delegate_->DetermineDownloadTarget(item, callback)) {
    base::FilePath target_path = item->GetForcedFilePath();
    // TODO(asanka): Determine a useful path if |target_path| is empty.
    callback.Run(target_path,
                 DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 target_path);
  }
}

bool DownloadManagerImpl::ShouldCompleteDownload(
    DownloadItemImpl* item, const base::Closure& complete_callback) {
  if (!delegate_ ||
      delegate_->ShouldCompleteDownload(item, complete_callback)) {
    return true;
  }
  // Otherwise, the delegate has accepted responsibility to run the
  // callback when the download is ready for completion.
  return false;
}

bool DownloadManagerImpl::ShouldOpenFileBasedOnExtension(
    const base::FilePath& path) {
  if (!delegate_)
    return false;

  return delegate_->ShouldOpenFileBasedOnExtension(path);
}

bool DownloadManagerImpl::ShouldOpenDownload(
    DownloadItemImpl* item, const ShouldOpenDownloadCallback& callback) {
  if (!delegate_)
    return true;

  // Relies on DownloadItemImplDelegate::ShouldOpenDownloadCallback and
  // DownloadManagerDelegate::DownloadOpenDelayedCallback "just happening"
  // to have the same type :-}.
  return delegate_->ShouldOpenDownload(item, callback);
}

void DownloadManagerImpl::SetDelegate(DownloadManagerDelegate* delegate) {
  delegate_ = delegate;
}

DownloadManagerDelegate* DownloadManagerImpl::GetDelegate() const {
  return delegate_;
}

void DownloadManagerImpl::Shutdown() {
  DVLOG(20) << __func__ << "() shutdown_needed_ = " << shutdown_needed_;
  if (!shutdown_needed_)
    return;
  shutdown_needed_ = false;

  FOR_EACH_OBSERVER(Observer, observers_, ManagerGoingDown(this));
  // TODO(benjhayden): Consider clearing observers_.

  // If there are in-progress downloads, cancel them. This also goes for
  // dangerous downloads which will remain in history if they aren't explicitly
  // accepted or discarded. Canceling will remove the intermediate download
  // file.
  for (const auto& it : downloads_) {
    DownloadItemImpl* download = it.second;
    if ((download->GetState() == DownloadItem::IN_PROGRESS ||
      download->GetState() == DownloadItem::PAUSING) &&
      (download->GetInternalState() != DownloadItemImpl::RESUMING_INTERNAL)) {
      download->Cancel(false);
    }
  }

#ifdef huk
  for (const auto& it : SavePackage_downloads_) {
    DownloadItemImpl* download = it.second;
    if ((download->GetState() == DownloadItem::IN_PROGRESS ||
      download->GetState() == DownloadItem::PAUSING) &&
      (download->GetInternalState() != DownloadItemImpl::RESUMING_INTERNAL)) {
      download->Cancel(false);
    }
  }
#endif

  base::STLDeleteValues(&downloads_);
 // huk  base::STLDeleteValues(&SavePackage_downloads_);
  downloads_by_guid_.clear();
  url_downloaders_.clear();

  // We'll have nothing more to report to the observers after this point.
  observers_.Clear();

//#if defined(OS_LINUX)
  if (delegate_ && !delegate_->IsOffTheRecord()) {
    DownloadFileImplSpeed::ApxDownloadExit();
  }
//#endif

  if (delegate_)
    delegate_->Shutdown();
  delegate_ = NULL;
}

void DownloadManagerImpl::CreateDownload(
    std::unique_ptr<DownloadCreateInfo> info,
    std::unique_ptr<ByteStreamReader> stream,
    const DownloadUrlParameters::OnStartedCallback& on_started) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(info);

  // |stream| is only non-nil if the download request was successful.
  //DCHECK((info->result == DOWNLOAD_INTERRUPT_REASON_NONE && stream.get()) ||
  //       (info->result != DOWNLOAD_INTERRUPT_REASON_NONE && !stream.get()));
  DVLOG(20) << __func__
            << "() result=" << DownloadInterruptReasonToString(info->result);
  uint32_t download_id = info->download_id;

  const bool new_download = (download_id == content::DownloadItem::kInvalidId);
  const bool force_speedy = info->force_speedy;

  if (new_download)
    RecordDownloadConnectionSecurity(info->url(), info->url_chain);
  base::Callback<void(uint32_t)> got_id(base::Bind(
      &DownloadManagerImpl::StartDownloadWithId, weak_factory_.GetWeakPtr(),
      base::Passed(&info), base::Passed(&stream), on_started, new_download, force_speedy));

  if (new_download) {
    GetNextId(got_id);
  } else {
    got_id.Run(download_id);
  }
}

void DownloadManagerImpl::StartDownload(
    std::unique_ptr<DownloadCreateInfo> info,
    std::unique_ptr<ByteStreamReader> stream,
    const DownloadUrlParameters::OnStartedCallback& on_started) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!ShouldUseSaveAsDownloadMode(info->url_chain.back(), false)) {
    if (info && info->request_handle) {
      content::WebContents* web_contents = info->request_handle->GetWebContents();
      if (delegate_ && web_contents) {
        DownloadVector downloads;
        GetAllDownloads(&downloads);
        base::FilePath default_download_directory = GetDefaultDownloadPath();
        info->save_info->file_path = default_download_directory;
        info->save_info->suggested_name = GetSuggestedFileName(
          info->url_chain.back(),
          info->mime_type,
          info->content_disposition,
          delegate_->GetReferrerCharset(),
          delegate_->GetDownloadDefaultFileName(),
          default_download_directory,
          &downloads);
        // check should prompt for download
        if (delegate_->ShouldPromptForDownload()) {
          info->request_handle->CancelRequest();
          delegate_->ShowCreateDownload(web_contents);
          FOR_EACH_OBSERVER(Observer, observers_, OnDownloadPromptd(this, info.get()));
        } else {
 #if defined(OS_POSIX)
           info->save_info->file_path = info->save_info->file_path.Append(
             base::UTF16ToUTF8(info->save_info->suggested_name));
 #elif defined(OS_WIN)
           info->save_info->file_path = info->save_info->file_path.Append(
             info->save_info->suggested_name);
 #endif
          CreateDownload(std::move(info), std::move(stream), on_started);
        }
      }
    }
  } else {
    CreateDownload(std::move(info), std::move(stream), on_started);
  }
}

void DownloadManagerImpl::StartDownloadWithId(
    std::unique_ptr<DownloadCreateInfo> info,
    std::unique_ptr<ByteStreamReader> stream,
    const DownloadUrlParameters::OnStartedCallback& on_started,
    bool new_download,
    bool force_speedy,
    uint32_t id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_NE(content::DownloadItem::kInvalidId, id);
  DownloadItemImpl* download = NULL;
  if (new_download) {
    download = CreateActiveItem(id, *info);
  } else {
    DownloadMap::iterator item_iterator = downloads_.find(id);
    // Trying to resume an interrupted download.
    if (item_iterator == downloads_.end() ||
        (item_iterator->second->GetState() == DownloadItem::CANCELLED)) {
      // If the download is no longer known to the DownloadManager, then it was
      // removed after it was resumed. Ignore. If the download is cancelled
      // while resuming, then also ignore the request.
      if(info->request_handle)
        info->request_handle->CancelRequest();
      if (!on_started.is_null())
        on_started.Run(NULL, DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
      // The ByteStreamReader lives and dies on the FILE thread.
      if (info->result == DOWNLOAD_INTERRUPT_REASON_NONE)
        BrowserThread::DeleteSoon(BrowserThread::FILE, FROM_HERE,
                                  stream.release());
      return;
    }

    download = item_iterator->second;
  }
  base::FilePath default_download_directory = GetDefaultDownloadPath();

  std::unique_ptr<DownloadFile> download_file;
  if (info->result == DOWNLOAD_INTERRUPT_REASON_NONE) {
    if (!ShouldUseSpeedDownloadMode(download)  && !force_speedy) {
      download_file.reset(
        file_factory_->CreateFile(std::move(info->save_info),
                                  default_download_directory,
                                  std::move(stream),
                                  download->GetNetLogWithSource(),
                                  download->DestinationObserverAsWeakPtr()));
    } else {
      download_file.reset(
        file_factory_->CreateFileSpeed(download->GetId(),
                                       download->GetTotalBytes(),
                                       download->GetReceivedBytes(),
                                       GetCookies(download->GetTabUrl()),
                                       download->GetURL(),
                                       download->GetReferrerUrl(),
                                       info->method,
                                       info->pack_url,
                                       std::move(info->save_info),
                                       info->auth_credential,
                                       default_download_directory,
                                       std::move(stream),
                                       download->GetNetLogWithSource(),
                                       download->DestinationObserverAsWeakPtr(),
                                       false, false, this));
    }
  }
  std::unique_ptr<DownloadRequestHandleInterface> request_handle_ptr;
  if (!ShouldUseSpeedDownloadMode(download) && !force_speedy) {
    request_handle_ptr = std::move(info->request_handle);
  } else {
    // cancel the origin request first and use the speed core
    if (info->request_handle.get()) {
      info->request_handle->CancelRequest();
    }
    DownloadFileImplSpeed* download_file_speed = (DownloadFileImplSpeed*)(download_file.get());
    DownloadRequestHandleInterface* handler = (DownloadRequestHandleInterface*)(download_file_speed);
    request_handle_ptr.reset(handler);
  }

  // It is important to leave info->save_info intact in the case of an interrupt
  // so that the DownloadItem can salvage what it can out of a failed resumption
  // attempt.

  // Start the download.
  download->Start(std::move(download_file), std::move(request_handle_ptr), *info);

  // For interrupted downloads, Start() will transition the state to
  // IN_PROGRESS and consumers will be notified via OnDownloadUpdated().
  // For new downloads, we notify here, rather than earlier, so that
  // the download_file is bound to download and all the usual
  // setters (e.g. Cancel) work.
 // if (!download->IsSaveAsDownloadMode()) { // huk
    if (new_download) {
      FOR_EACH_OBSERVER(Observer, observers_, OnDownloadCreated(this, download));
    } else {
      FOR_EACH_OBSERVER(Observer, observers_, OnDownloadReCreated(this, download));
    }
  //}

  if (!on_started.is_null())
    on_started.Run(download, DOWNLOAD_INTERRUPT_REASON_NONE);
}

void DownloadManagerImpl::CheckForHistoryFilesRemoval() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (const auto& it : downloads_) {
    DownloadItemImpl* item = it.second;
    CheckForFileRemoval(item);
  }
}

void DownloadManagerImpl::CheckForFileRemoval(DownloadItemImpl* download_item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if ((download_item->GetState() == DownloadItem::COMPLETE) &&
      !download_item->GetFileExternallyRemoved() &&
      delegate_) {
    delegate_->CheckForFileExistence(
        download_item,
        base::Bind(&DownloadManagerImpl::OnFileExistenceChecked,
                   weak_factory_.GetWeakPtr(), download_item->GetId()));
  }
}

void DownloadManagerImpl::OnFileExistenceChecked(uint32_t download_id,
                                                 bool result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!result) {  // File does not exist.
    if (base::ContainsKey(downloads_, download_id))
      downloads_[download_id]->OnDownloadedFileRemoved();
  }
}

std::string DownloadManagerImpl::GetApplicationClientIdForFileScanning() const {
  if (delegate_)
    return delegate_->ApplicationClientIdForFileScanning();
  return std::string();
}

bool DownloadManagerImpl::ProhibitConcat(std::string url) {
  return false;
}

BrowserContext* DownloadManagerImpl::GetBrowserContext() const {
  return browser_context_;
}

void DownloadManagerImpl::CreateSavePackageDownloadItem(
    const base::FilePath& main_file_path,
    const GURL& page_url,
    const std::string& mime_type,
    std::unique_ptr<DownloadRequestHandleInterface> request_handle,
    const DownloadItemImplCreated& item_created) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetNextId(base::Bind(
      &DownloadManagerImpl::CreateSavePackageDownloadItemWithId,
      weak_factory_.GetWeakPtr(), main_file_path, page_url, mime_type,
      base::Passed(std::move(request_handle)), item_created));
}

void DownloadManagerImpl::CreateSavePackageDownloadItemWithId(
    const base::FilePath& main_file_path,
    const GURL& page_url,
    const std::string& mime_type,
    std::unique_ptr<DownloadRequestHandleInterface> request_handle,
    const DownloadItemImplCreated& item_created,
    uint32_t id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_NE(content::DownloadItem::kInvalidId, id);
  DCHECK(!base::ContainsKey(downloads_, id));
  net::NetLogWithSource net_log =
      net::NetLogWithSource::Make(net_log_, net::NetLogSourceType::DOWNLOAD);
  DownloadItemImpl* download_item = item_factory_->CreateSavePageItem(
      this, id, main_file_path, page_url, mime_type, std::move(request_handle),
      net_log);

  // SavePackage_downloads_[download_item->GetId()] = download_item;
  downloads_[download_item->GetId()] = download_item;
  // DCHECK(!base::ContainsKey(downloads_by_guid_, download_item->GetGuid()));
  // downloads_by_guid_[download_item->GetGuid()] = download_item;
  FOR_EACH_OBSERVER(Observer, observers_, OnDownloadCreated(
     this, download_item));
  if (!item_created.is_null())
    item_created.Run(download_item);
}

void DownloadManagerImpl::OnSavePackageSuccessfullyFinished(
    DownloadItem* download_item) {
  FOR_EACH_OBSERVER(Observer, observers_,
                   OnSavePackageSuccessfullyFinished(this, download_item));
}

// Resume a download of a specific URL. We send the request to the
// ResourceDispatcherHost, and let it send us responses like a regular
// download.
void DownloadManagerImpl::ResumeInterruptedDownload(
    std::unique_ptr<content::DownloadUrlParameters> params,
    uint32_t id) {
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&BeginDownload, base::Passed(&params),
                 browser_context_->GetResourceContext(), id,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&DownloadManagerImpl::AddUrlDownloader,
                 weak_factory_.GetWeakPtr()));
}

void DownloadManagerImpl::ResumeInterruptedDownloadEx(
    DownloadItemImpl* download,
    const DownloadUrlParameters::OnStartedCallback& on_started) {
  CHECK(download);
  download->Reset();
  std::unique_ptr<content::DownloadCreateInfo> info(new content::DownloadCreateInfo);
  std::unique_ptr<content::ByteStreamReader> stream = NULL;
  info->download_id = download->GetId();
  info->url_chain.push_back(download->GetURL());
  info->referrer_url = download->GetReferrerUrl();
  info->tab_url = download->GetTabUrl();
  info->method = download->GetMethod();
  info->pack_url = download->GetPackUrl();
  info->save_info->suggested_name = base::UTF8ToUTF16(download->GetSuggestedFilename());
  info->save_info->file_path = download->GetTargetFilePath();
  info->result = content::DOWNLOAD_INTERRUPT_REASON_NONE;
  info->start_time = base::Time::Now();

  CreateDownload(std::move(info), std::move(stream), on_started);
}

void DownloadManagerImpl::SetDownloadItemFactoryForTesting(
    std::unique_ptr<DownloadItemFactory> item_factory) {
  item_factory_ = std::move(item_factory);
}

void DownloadManagerImpl::SetDownloadFileFactoryForTesting(
    std::unique_ptr<DownloadFileFactory> file_factory) {
  file_factory_ = std::move(file_factory);
}

DownloadFileFactory* DownloadManagerImpl::GetDownloadFileFactoryForTesting() {
  return file_factory_.get();
}

void DownloadManagerImpl::DownloadRemoved(DownloadItemImpl* download) {
  if (!download)
    return;

  downloads_by_guid_.erase(download->GetGuid());

  uint32_t download_id = download->GetId();
  if (downloads_.erase(download_id) == 0)
    return;
  delete download;
}

void DownloadManagerImpl::DownloadCompleted(DownloadItemImpl* download) {
   // (huk)  把文件名和路径写到本地tmp文件中，为漏洞扫描组提供支持
  #if !defined(OS_WIN)
  if (download != NULL)  {
    std::string file_path = download->GetTargetFilePath().value();
    file_path += "\n";
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&WriteInfoIntoTmpFile, file_path));
  }
  #endif

  // if (download->IsSaveAsDownloadMode()) {
  //   return;
  // }

  if (delegate_) {
    delegate_->DownloadCompleted(download);
  }
}

bool DownloadManagerImpl::ShouldUseSaveAsDownloadMode(GURL url, bool find_delete) {
  return true;

  if (url.SchemeIsData() || url.SchemeIsBlob() || url.SchemeIsFile()) {
    return true;
  }

  if (CheckIsSaveTypeDownload(url, find_delete)) {
    return true;
  }

  // mantis
  if (url.HostIsIPAddress())  {
    return true;
  }

  return false;
}

bool DownloadManagerImpl::ShouldUseSpeedDownloadMode(DownloadItemImpl* download) {
  return false;

  DCHECK(download);
//#if !defined(OS_LINUX)
//  return false;
//#else
  // for some resources we should use the default download mode
  // such as: save package, data/blob protocol, pictures...
  if (download->IsSaveAsDownloadMode() || download->GetURL().HostIsIPAddress()) {
    return false;
  }

  return true;
//#endif
}

void DownloadManagerImpl::AddUrlDownloader(
    std::unique_ptr<UrlDownloader, BrowserThread::DeleteOnIOThread>
        downloader) {
  if (downloader)
    url_downloaders_.push_back(std::move(downloader));
}

void DownloadManagerImpl::RemoveUrlDownloader(UrlDownloader* downloader) {
  for (auto ptr = url_downloaders_.begin(); ptr != url_downloaders_.end();
       ++ptr) {
    if (ptr->get() == downloader) {
      url_downloaders_.erase(ptr);
      return;
    }
  }
}

static void GetCookiesCallback(std::string* cookies_out,
    base::WaitableEvent* event,
    const std::string& cookies) {
  *cookies_out = cookies;
  event->Signal();
}

static void GetCookiesOnIOThread(const GURL& url,
    net::URLRequestContextGetter* context_getter,
    base::WaitableEvent* event,
    std::string* cookies) {
  net::CookieStore* cookie_store =
      context_getter->GetURLRequestContext()->cookie_store();
  net::CookieOptions options = net::CookieOptions();
  options.set_include_httponly();
  cookie_store->GetCookiesWithOptionsAsync(
      url, options,
      base::Bind(&GetCookiesCallback, cookies, event));
}

std::string DownloadManagerImpl::GetCookies(const GURL& tab_url) {
  std::string cookies;
  //base::WaitableEvent event(true, false);
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
  net::URLRequestContextGetter* context_getter =
       BrowserContext::GetDefaultStoragePartition(GetBrowserContext())->GetURLRequestContext();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&GetCookiesOnIOThread, tab_url,
                 base::RetainedRef(context_getter), &event, &cookies));
  bool original_wait_allowed = base::ThreadRestrictions::SetWaitAllowed(true);
  event.Wait();
  base::ThreadRestrictions::SetWaitAllowed(original_wait_allowed);
  return cookies;
}

// static
DownloadInterruptReason DownloadManagerImpl::BeginDownloadRequest(
    std::unique_ptr<net::URLRequest> url_request,
    const Referrer& referrer,
    ResourceContext* resource_context,
    bool is_content_initiated,
    int render_process_id,
    int render_view_route_id,
    int render_frame_route_id,
    bool do_not_prompt_for_login) {
  if (ResourceDispatcherHostImpl::Get()->is_shutdown())
    return DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN;

  // The URLRequest needs to be initialized with the referrer and other
  // information prior to issuing it.
  ResourceDispatcherHostImpl::Get()->InitializeURLRequest(
      url_request.get(), referrer,
      true,  // download.
      render_process_id, render_view_route_id, render_frame_route_id,
      resource_context);

  // We treat a download as a main frame load, and thus update the policy URL on
  // redirects.
  //
  // TODO(davidben): Is this correct? If this came from a
  // ViewHostMsg_DownloadUrl in a frame, should it have first-party URL set
  // appropriately?
  url_request->set_first_party_url_policy(
      net::URLRequest::UPDATE_FIRST_PARTY_URL_ON_REDIRECT);

  const GURL& url = url_request->original_url();

  // Check if the renderer is permitted to request the requested URL.
  if (!ChildProcessSecurityPolicyImpl::GetInstance()->CanRequestURL(
          render_process_id, url)) {
    DVLOG(1) << "Denied unauthorized download request for "
             << url.possibly_invalid_spec();
    return DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST;
  }

  const net::URLRequestContext* request_context = url_request->context();
  if (!request_context->job_factory()->IsHandledURL(url)) {
    DVLOG(1) << "Download request for unsupported protocol: "
             << url.possibly_invalid_spec();
    return DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST;
  }

  // From this point forward, the |DownloadResourceHandler| is responsible for
  // |started_callback|.
  // TODO(ananta)
  // Find a better way to create the DownloadResourceHandler instance.
  std::unique_ptr<ResourceHandler> handler(
      DownloadResourceHandler::Create(url_request.get()));

  ResourceDispatcherHostImpl::Get()->BeginURLRequest(
      std::move(url_request), std::move(handler), true,  // download
      is_content_initiated, do_not_prompt_for_login, resource_context);
  return DOWNLOAD_INTERRUPT_REASON_NONE;
}

namespace {

bool EmptyFilter(const GURL& url) {
  return true;
}

bool RemoveDownloadByURLAndTime(
    const base::Callback<bool(const GURL&)>& url_filter,
          base::Time remove_begin,
          base::Time remove_end,
          const DownloadItemImpl* download_item) {
  return url_filter.Run(download_item->GetURL()) &&
         download_item->GetStartTime() >= remove_begin &&
         (remove_end.is_null() || download_item->GetStartTime() < remove_end);
}

}  // namespace

int DownloadManagerImpl::RemoveDownloads(const DownloadRemover& remover) {
  int count = 0;
  DownloadMap::const_iterator it = downloads_.begin();
  while (it != downloads_.end()) {
    DownloadItemImpl* download = it->second;

    // Increment done here to protect against invalidation below.
    ++it;

    if (download->GetState() != DownloadItem::IN_PROGRESS &&
        remover.Run(download)) {
      download->Remove();
      count++;
    }
  }
  return count;
}

int DownloadManagerImpl::RemoveDownloadsByURLAndTime(
    const base::Callback<bool(const GURL&)>& url_filter,
    base::Time remove_begin,
    base::Time remove_end) {
  return RemoveDownloads(base::Bind(&RemoveDownloadByURLAndTime,
                                    url_filter,
                                    remove_begin, remove_end));
}

int DownloadManagerImpl::RemoveAllDownloads() {
  const base::Callback<bool(const GURL&)> empty_filter =
      base::Bind(&EmptyFilter);
  // The null times make the date range unbounded.
  int num_deleted = RemoveDownloadsByURLAndTime(
      empty_filter, base::Time(), base::Time());
  RecordClearAllSize(num_deleted);
  return num_deleted;
}

void DownloadManagerImpl::DownloadUrl(
    std::unique_ptr<DownloadUrlParameters> params) {
  if (params->post_id() >= 0) {
    // Check this here so that the traceback is more useful.
    DCHECK(params->prefer_cache());
    DCHECK_EQ("POST", params->method());
  }

  AddDownloadToDataList(params->url(), params->url_source_from());
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&BeginDownload, base::Passed(&params),
                 browser_context_->GetResourceContext(),
                 content::DownloadItem::kInvalidId, weak_factory_.GetWeakPtr()),
      base::Bind(&DownloadManagerImpl::AddUrlDownloader,
                 weak_factory_.GetWeakPtr()));
}

void DownloadManagerImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DownloadManagerImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

DownloadItem* DownloadManagerImpl::CreateDownloadItem(
    const std::string& guid,
    uint32_t id,
    const base::FilePath& current_path,
    const base::FilePath& target_path,
    const std::vector<GURL>& url_chain,
    const GURL& referrer_url,
    const GURL& site_url,
    const GURL& tab_url,
    const GURL& tab_refererr_url,
    const std::string& method,
    const std::string& pack_url,
    const std::string& mime_type,
    const std::string& original_mime_type,
    const base::Time& start_time,
    const base::Time& end_time,
    const std::string& etag,
    const std::string& last_modified,
    const std::string& bt_real_name,
    int64_t received_bytes,
    int64_t total_bytes,
    const std::string& hash,
    DownloadItem::DownloadState state,
    DownloadDangerType danger_type,
    DownloadInterruptReason interrupt_reason,
    bool opened) {
  if (base::ContainsKey(downloads_, id)) {
    NOTREACHED();
    return NULL;
  }

  DCHECK(!base::ContainsKey(downloads_by_guid_, guid));
  DownloadItemImpl* item = item_factory_->CreatePersistedItem(
      this, guid, id, current_path, target_path, url_chain, referrer_url,
      site_url, tab_url, tab_refererr_url, method, pack_url, mime_type, original_mime_type,
      start_time, end_time, etag, last_modified, bt_real_name, received_bytes, total_bytes,
      hash, state, danger_type, interrupt_reason, opened,
      net::NetLogWithSource::Make(net_log_, net::NetLogSourceType::DOWNLOAD));
  downloads_[id] = item;
  downloads_by_guid_[guid] = item;
  FOR_EACH_OBSERVER(Observer, observers_, OnDownloadCreated(this, item));
  DVLOG(20) << __func__ << "() download = " << item->DebugString(true);

  // suport breakpoint resume
//#if defined(OS_LINUX)
  if (state == content::DownloadItem::PAUSING) {
    base::FilePath default_download_directory = GetDefaultDownloadPath();
    std::unique_ptr<DownloadCreateInfo> info;
    info.reset(new DownloadCreateInfo());
    info->save_info = std::unique_ptr<DownloadSaveInfo>(new DownloadSaveInfo());
    info->save_info->prompt_for_save_location = false;
    info->download_id = item->GetId();

    bool task_status = false;
    if(delegate_ && !delegate_->ShouldContinueLastDownload())
      task_status = true;

    std::unique_ptr<DownloadFile> download_file;
    download_file.reset(file_factory_->CreateFileSpeed(item->GetId(),
                                    item->GetTotalBytes(),
                                    item->GetReceivedBytes(),
                                    GetCookies(item->GetTabUrl()),
                                    item->GetURL(),
                                    item->GetReferrerUrl(),
                                    info->method,
                                    info->pack_url,
                                    std::move(info->save_info),
                                    info->auth_credential,
                                    default_download_directory,
                                    NULL,
                                    item->GetNetLogWithSource(),
                                    item->DestinationObserverAsWeakPtr(),
                                    task_status, true, this));

    std::unique_ptr<DownloadRequestHandleInterface> request_handle_ptr;
    DownloadFileImplSpeed* download_file_speed = (DownloadFileImplSpeed*)(download_file.get());
    DownloadRequestHandleInterface* handler = (DownloadRequestHandleInterface*)(download_file_speed);
    request_handle_ptr.reset(handler);

    if (task_status) {
      //keep pausing
      item->KeepPause(std::move(download_file), std::move(request_handle_ptr));
    } else {
      // Start the download.
      item->Start(std::move(download_file), std::move(request_handle_ptr), *info);
    }
  }
//#endif

  return item;
}

int DownloadManagerImpl::InProgressCount() const {
  int count = 0;
  for (const auto& it : downloads_) {
    if (it.second->GetState() == DownloadItem::IN_PROGRESS)
      ++count;
  }
  return count;
}

int DownloadManagerImpl::NonMaliciousInProgressCount() const {
  int count = 0;
  if(delegate_ && !delegate_->ShouldNoteDownloadQuit())
    return count;

  for (const auto& it : downloads_) {
    if (it.second->GetState() == DownloadItem::IN_PROGRESS &&
        it.second->GetDangerType() != DOWNLOAD_DANGER_TYPE_DANGEROUS_URL &&
        it.second->GetDangerType() != DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT &&
        it.second->GetDangerType() != DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST &&
        it.second->GetDangerType() !=
            DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED) {
      ++count;
    }
  }
  return count;
}

DownloadItem* DownloadManagerImpl::GetDownload(uint32_t download_id) {
  if (base::ContainsKey(downloads_, download_id)) {
    return downloads_[download_id];
  }

  if (base::ContainsKey(SavePackage_downloads_, download_id)) {
    return SavePackage_downloads_[download_id];
  }

  return nullptr;
}

DownloadItem* DownloadManagerImpl::GetDownloadByGuid(const std::string& guid) {
  DCHECK(guid == base::ToUpperASCII(guid));
  return base::ContainsKey(downloads_by_guid_, guid) ? downloads_by_guid_[guid]
                                                     : nullptr;
}

base::FilePath DownloadManagerImpl::GetDefaultDownloadPath() {
  base::FilePath default_download_directory;
  if (delegate_) {
    base::FilePath website_save_directory;  // Unused
    bool skip_dir_check = false;            // Unused
    delegate_->GetSaveDir(GetBrowserContext(), &website_save_directory,
                          &default_download_directory, &skip_dir_check);
  }

  return default_download_directory;
}

void DownloadManagerImpl::GetAllDownloads(DownloadVector* downloads) {
  for (const auto& it : downloads_) {
    downloads->push_back(it.second);
  }
}

void DownloadManagerImpl::OpenDownload(DownloadItemImpl* download) {
  int num_unopened = 0;
  string16 extension = download->GetFullPath().Extension();
  // huk disable bt (not stable) todo: bring it back
  bool bt_enabled = base::CommandLine::ForCurrentProcess()->HasSwitch(
          "enable_bt");

  if (bt_enabled && !extension.empty() && extension == STRING_LITERAL(".torrent"))  {  //bt
     bool previous_disallowed = base::ThreadRestrictions::SetIOAllowed(true);
     if (download->GetBtRealFileName().empty()) {
       char bt_fname[256];
       string16 bt_url = download->GetFullPath().value();
       DownloadFileImplSpeed::ApxBtFileParse((char*)bt_url.c_str(), bt_fname, sizeof(bt_fname) - 1);
       download->SetBtRealFileName(bt_fname);
     }

     base::FilePath target_path;
     if (download->GetBtRealFileName().empty()) {
       target_path = download->GetTargetFilePath().RemoveExtension();;
     } else {
 #if defined(OS_POSIX)
       target_path = download->GetTargetFilePath().DirName().Append(
         download->GetBtRealFileName());
 #elif defined(OS_WIN)
       target_path = download->GetTargetFilePath().DirName().Append(
         base::UTF8ToWide(download->GetBtRealFileName()));
 #endif
     }

     if (download->GetBtRealFileName().empty() || !base::DirectoryExists(target_path)) {
       base::ThreadRestrictions::SetIOAllowed(previous_disallowed);
       string16 bt_url =  STRING_LITERAL("bt://") + download->GetFullPath().value();
       std::unique_ptr<content::DownloadCreateInfo> info(new content::DownloadCreateInfo);
       info->url_chain.push_back(GURL(bt_url));
       info->save_info->file_path = download->GetFullPath().RemoveExtension();
       info->result = content::DOWNLOAD_INTERRUPT_REASON_NONE;
       info->start_time = base::Time::Now();
       info->download_parent = download;

       std::unique_ptr<content::ByteStreamReader> stream = NULL;
       DownloadUrlParameters::OnStartedCallback on_started;
       CreateDownload(std::move(info), std::move(stream), on_started);
       return;
     }

     base::ThreadRestrictions::SetIOAllowed(previous_disallowed);
  }

  for (const auto& it : downloads_) {
    DownloadItemImpl* item = it.second;
    if ((item->GetState() == DownloadItem::COMPLETE) &&
        !item->GetOpened())
      ++num_unopened;

    if (!extension.empty() && extension == STRING_LITERAL(".torrent") && item != download &&
        item->GetBtRealFileName() == download->GetBtRealFileName()) {
      item->ShowDownloadInShell();
      return;
    }
  }

  RecordOpensOutstanding(num_unopened);

  if (delegate_)
    delegate_->OpenDownload(download);
}

void DownloadManagerImpl::ShowDownloadInShell(DownloadItemImpl* download) {
  if (delegate_)
    delegate_->ShowDownloadInShell(download);
}

void DownloadManagerImpl::AddDownloadToDataList(const GURL& url,
    DownloadUrlParameters::UrlSourceFrom from) {
  url_source_from_data_list_.push_back(new UrlSourceFromData(url, from));
}

bool CompareDownloadURL(const GURL origin, const GURL current) {
  std::string origin_url = origin.spec();
  std::string current_url = current.spec();

  if (origin_url[origin_url.length() - 1] == '/') {
    origin_url = origin_url.substr(0, origin_url.length() - 1);
  }

  if (current_url[current_url.length() - 1] == '/') {
    current_url = current_url.substr(0, current_url.length() - 1);
  }

  return (origin_url == current_url);
}

bool DownloadManagerImpl::CheckIsSaveTypeDownload(const GURL& url, bool find_delete) {
  std::vector<UrlSourceFromData*>::reverse_iterator iter =
  url_source_from_data_list_.rbegin();
  while (iter != url_source_from_data_list_.rend()) {
    if (CompareDownloadURL((*iter)->source_url, url)) {
      DownloadUrlParameters::UrlSourceFrom from = (*iter)->source_from;
      if (from == DownloadUrlParameters::URL_SOURCE_FROM_SAVEAS ||
          from == DownloadUrlParameters::URL_SOURCE_FROM_SAVELINKSAS ||
          from == DownloadUrlParameters::URL_SOURCE_FROM_SAVEIMAGEAS) {
        if (find_delete) {
          delete (*iter);
          url_source_from_data_list_.erase((++iter).base());
        }

        return true;
      } else {
        return false;
      }
    }

    ++iter;
  }

  return false;
}

void DownloadManagerImpl::ClearUrlSourceFromList() {
  std::vector<UrlSourceFromData*>::iterator iter =
  url_source_from_data_list_.begin();
  while (iter != url_source_from_data_list_.end()) {
    delete (*iter);
    url_source_from_data_list_.erase(iter++);
  }
}

}  // namespace content
