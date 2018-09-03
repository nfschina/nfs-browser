// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_file_impl_speed.h"

#include <string>
#include <utility>

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <pthread.h>
#endif

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "content/browser/byte_stream.h"
#include "content/public/browser/download_manager_delegate.h" //add by luyue
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_destination_observer.h"
#include "content/browser/download/download_interrupt_reasons_impl.h"
#include "content/browser/download/download_net_log_parameters.h"
#include "content/browser/download/download_stats.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "net/base/io_buffer.h"
#include "net/base/filename_util.h"
#include "base/strings/utf_string_conversions.h"

extern "C"{
// include the speed apx_hftsc_api.h file
#include "content/browser/download/speed_download/include/apx_hftsc_api.h"
}

#if defined(OS_POSIX)
#define STRING_LITERAL(x)
typedef std::string string16;
#elif defined(OS_WIN)
#define STRING_LITERAL(x) L ## x
typedef std::wstring StringPieceType;
#endif

namespace content {

const int kUpdatePeriodMs = 1000;
const int kMaxTimeBlockingFileThreadMs = 5;

// These constants control the default retry behavior for failing renames. Each
// retry is performed after a delay that is twice the previous delay. The
// initial delay is specified by kInitialRenameRetryDelayMs.
const int kInitialRenameRetryDelayMs = 200;

// Number of times a failing rename is retried before giving up.
const int kMaxRenameRetries = 3;

//network status 0:OK
static int kNetworkStatus = 0;
volatile static bool NetWorkThreadClose = false;

#if defined(OS_WIN)
//static Handle  NetworkId;
#elif defined(OS_POSIX)
static pthread_t  NetworkId;
#endif

#if defined(OS_WIN)
//static const int kThreadInitialStackSize = 64 * 1024;
#endif

#if defined(OS_WIN)
//typedef void* Handle;
//static HANDLE mutex;
#elif defined(OS_POSIX)
typedef pthread_t Handle;
 pthread_cond_t cond;
 pthread_mutex_t mutex;
#endif

// apx init state
static bool isApxDownloadInited = false;

DownloadFileImplSpeed::DownloadFileImplSpeed(uint32_t download_id,
                                             int64_t bytes_total,
                                             int64_t bytes_received,
                                             std::string cookies,
                                             GURL url_chain,
                                             GURL referrer_url,
                                             std::string method,
                                             std::string pack_url,
                                             std::unique_ptr<DownloadSaveInfo> save_info,
                                             net::AuthCredentials auth_credential,
                                             const base::FilePath& default_download_directory,
                                             std::unique_ptr<ByteStreamReader> stream,
                                             const net::NetLogWithSource& bound_net_log,
                                             base::WeakPtr<DownloadDestinationObserver> observer,
                                             bool task_status,
                                             bool is_breakpoint,
                                             DownloadManager* download_manager)
    : file_(bound_net_log),
      download_id_(download_id),
      cookies_(cookies),
      url_chain_(url_chain),
      referrer_url_(referrer_url),
      method_(method),
      pack_url_(pack_url),
      bytes_so_far_(bytes_received),
      bytes_total_(bytes_total),
      count_per_second_(0),
      save_info_(std::move(save_info)),
      default_download_directory_(default_download_directory),
      target_path_(save_info_->file_path),
      stream_reader_(std::move(stream)),
      download_manager_(download_manager),
      bytes_seen_(0),
      bound_net_log_(bound_net_log),
      observer_(observer),
      task_status_(task_status),
      is_breakpoint_(is_breakpoint),
      auth_credential_(auth_credential),
      weak_factory_(this) {
        download_splimit_ = UpdateDownloadMaxSpeed();
        is_paused_ = task_status_;
      }

DownloadFileImplSpeed::~DownloadFileImplSpeed() {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  task_status_ = false;
}

#if defined(OS_WIN)
DWORD NetworkStatusGet(void* lpParameter) {
#elif defined(OS_POSIX)
void* NetworkStatusGet(void*) {
#endif
  apx_net_start();

  APX_NETWORK_E eth_result = apx_net_detect_interface();
  if(eth_result == APX_NET_INTER_ERR) {
    apx_net_end();
    return NULL;
  }
  APX_NETWORK_E ip_result = apx_net_detect_ip();
  if(ip_result == APX_NET_IP_UNSET || ip_result == APX_NET_UNKOWN) {
    apx_net_end();
    return NULL;
  }

  char *pAddr = new char[16];
  char *pGateWay = new char[16];

#if defined(OS_WIN)
  //::WaitForSingleObject(mutex, INFINITE);
#elif defined(OS_POSIX)
  struct timeval now;
  struct timespec outtime;
pthread_mutex_lock(&mutex);
#endif

  while (!NetWorkThreadClose) {
    APX_NETWORK_E route_result = apx_net_detect_route(pAddr,pGateWay);
    kNetworkStatus = route_result;
#if defined(OS_WIN)
    //Sleep(3000);
#elif defined(OS_POSIX)
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 3;
    outtime.tv_nsec = 0;

    pthread_cond_timedwait(&cond, &mutex, &outtime);
#endif
  }
#if defined(OS_WIN)
  //::ReleaseMutex(mutex);
#elif defined(OS_POSIX)
pthread_mutex_unlock(&mutex);
#endif
  delete[]pAddr;
  delete[]pGateWay;
  apx_net_end();
  return NULL;
}

void DownloadFileImplSpeed::ApxDownloadInit(base::FilePath user_data_path) {
  if (isApxDownloadInited) {
    return;
  }

  char path_conf[5000];
  #if defined(OS_WIN)
  user_data_path = user_data_path.Append(STRING_LITERAL("download"));
  strncpy(path_conf, base::WideToUTF8(user_data_path.value()).c_str(), sizeof(path_conf)-1);
  #elif defined(OS_POSIX)
  user_data_path = user_data_path.Append(std::string("download"));
  strncpy(path_conf, user_data_path.value().c_str(), sizeof(path_conf)-1);
  #endif
  apx_hftsc_init(path_conf);
  int uid = apx_user_login();
  if (uid < 0) {
    //printf("login failed. uid is %d. \n", uid);
  }

  if(apx_task_restore(uid) < 0) {
    //printf("restore task failed. uid is %d. \n", uid);
  }

  isApxDownloadInited = true;
  #if defined(OS_WIN)
  //mutex = ::CreateMutex(NULL, FALSE, NULL);
  //NetworkId = ::CreateThread(nullptr, kThreadInitialStackSize, NetworkStatusGet, NULL, 0, nullptr);
  //if (NetworkId) {
  //  CloseHandle(NetworkId);
  //}
  #elif defined(OS_POSIX)
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  int ret = pthread_create( &NetworkId, NULL, NetworkStatusGet, NULL );
  if (ret < 0) {
  }
  #endif
}

void DownloadFileImplSpeed::ApxDownloadExit() {
  if (!isApxDownloadInited) {
    return;
  }

  apx_task_exit();
  NetWorkThreadClose = true;
  #if defined(OS_WIN)
  //::ReleaseMutex(mutex);
  //::CloseHandle(mutex);
  #elif defined(OS_POSIX)

 // pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
 // pthread_mutex_unlock(&mutex);

 //pthread_join(NetworkId,NULL); //huk (ping takes 2-3s, we don't wanna the main thead to wait)
  #endif
  isApxDownloadInited = false;
}

void DownloadFileImplSpeed::ApxDownloadRemove(uint32_t download_id, bool delete_file) {
  int result = apx_task_delete(download_id);
  if (result < 0) {
    LOG(ERROR) << "ApxDownloadRemove delete task failed, result is : " << result;
  }

  if (delete_file) {
    int result = apx_task_destroy(download_id);
    if (result < 0) {
      LOG(ERROR) << "ApxDownloadRemove destroy task failed, result is : " << result;
    }
  }
}

int DownloadFileImplSpeed::ApxBtFileParse(char* bt_uri, char* bt_fname, int bt_fname_len) {
  return apx_task_btfile_parse(bt_uri, bt_fname, bt_fname_len);
}

// download file init
void DownloadFileImplSpeed::Initialize(const InitializeCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  update_timer_.reset(new base::RepeatingTimer());
  if (!update_timer_->IsRunning()) {
    update_timer_->Start(FROM_HERE,
                         base::TimeDelta::FromMilliseconds(kUpdatePeriodMs),
                         this, &DownloadFileImplSpeed::SendUpdate);
  }

  download_start_ = base::TimeTicks::Now();

  // Primarily to make reset to zero in restart visible to owner.
  SendUpdate();

  // create the apx task to download
  if(!task_status_) {
    ApxTaskCreate();
  }

  // Initial pull from the straw.
  StreamActive();

  // DownloadItemImpl::OnDownloadFileInitialized
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(
          callback, DOWNLOAD_INTERRUPT_REASON_NONE));
}

void DownloadFileImplSpeed::RenameAndUniquify(
    const base::FilePath& full_path,
    const RenameCompletionCallback& callback) {
  std::unique_ptr<RenameParameters> parameters(
      new RenameParameters(UNIQUIFY, full_path, callback));
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(parameters->completion_callback, DOWNLOAD_INTERRUPT_REASON_NONE, target_path_));
}

void DownloadFileImplSpeed::RenameAndAnnotate(
    const base::FilePath& full_path,
    const std::string& client_guid,
    const GURL& source_url,
    const GURL& referrer_url,
    const RenameCompletionCallback& callback) {
  std::unique_ptr<RenameParameters> parameters(new RenameParameters(
      ANNOTATE_WITH_SOURCE_INFORMATION, full_path, callback));
  parameters->client_guid = client_guid;
  parameters->source_url = source_url;
  parameters->referrer_url = referrer_url;
  RenameWithRetryInternal(std::move(parameters));
}

base::TimeDelta DownloadFileImplSpeed::GetRetryDelayForFailedRename(
    int attempt_number) {
  DCHECK_GE(attempt_number, 0);
  // |delay| starts at kInitialRenameRetryDelayMs and increases by a factor of
  // 2 at each subsequent retry. Assumes that |retries_left| starts at
  // kMaxRenameRetries. Also assumes that kMaxRenameRetries is less than the
  // number of bits in an int.
  return base::TimeDelta::FromMilliseconds(kInitialRenameRetryDelayMs) *
         (1 << attempt_number);
}

bool DownloadFileImplSpeed::ShouldRetryFailedRename(DownloadInterruptReason reason) {
  return reason == DOWNLOAD_INTERRUPT_REASON_FILE_TRANSIENT_ERROR;
}

void DownloadFileImplSpeed::RenameWithRetryInternal(
    std::unique_ptr<RenameParameters> parameters) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  char fpath[256]={0};
  char fname[256]={0};

  // if the file name is changed, then rename it
  base::FilePath real_path = parameters->new_path;
  int ret = apx_task_file_name_get(download_id_, fpath, sizeof(fpath), fname, sizeof(fname));
  if (ret == 0) {
#if defined(OS_WIN)
    base::FilePath file_path = base::FilePath(base::UTF8ToWide(fpath)).Append(
      base::UTF8ToWide(fname));
#elif defined(OS_POSIX)
    base::FilePath file_path = base::FilePath(fpath).Append(fname);
#endif
    if (file_path.value() != target_path_.value()) {
      target_path_ = file_path;
    }
  }

  // replace the download file from old path to new path
  base::File::Error error;
  if (!base::ReplaceFile(target_path_, real_path, &error)) {
    LOG(ERROR) << "Failed to rename target file. error=" << error;
    std::unique_ptr<crypto::SecureHash> hash_state = NULL;
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&DownloadDestinationObserver::DestinationError,
                   observer_,
                   DOWNLOAD_INTERRUPT_REASON_FILE_FAILED,
                   bytes_so_far_,
                   base::Passed(&hash_state)));
  }

  target_path_ = real_path;
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(parameters->completion_callback, DOWNLOAD_INTERRUPT_REASON_NONE, real_path));
}

void DownloadFileImplSpeed::Detach() {
}

void DownloadFileImplSpeed::Cancel() {
  int result = apx_task_delete(download_id_);
  if (result < 0) {
    LOG(ERROR) << "Cancel delete task failed, result is : " << result;
  }

  if (delete_file_) {
    int result = apx_task_destroy(download_id_);
    if (result < 0) {
      LOG(ERROR) << "Cancel destroy task failed, result is : " << result;
    }
  }

  task_status_ = false;
}

void DownloadFileImplSpeed::Delete() {
  delete_file_ = true;
}

const base::FilePath& DownloadFileImplSpeed::FullPath() const {
  return target_path_;
}

bool DownloadFileImplSpeed::InProgress() const {
  int state = apx_task_state_get(download_id_);
  return (state == APX_TASK_STATE_ACTIVE);
}

WebContents* DownloadFileImplSpeed::GetWebContents() const {
  return NULL;
}

DownloadManager* DownloadFileImplSpeed::GetDownloadManager() const {
  return download_manager_;
}

void DownloadFileImplSpeed::PauseRequest() {
  is_paused_ = true;
  int result = apx_task_stop(download_id_);
  if (result < 0) {
    LOG(ERROR) << "PauseRequest apx_task_stop failed, result is : " << result;
  }
}

void DownloadFileImplSpeed::ResumeRequest() {
  if(!task_status_) {
    ApxTaskCreate();
  }
  int result = apx_task_limit_set(download_id_,UpdateDownloadMaxSpeed());
  if (result < 0) {
    LOG(ERROR) << "set download speed failed, result is : " << result;
  }

  result = apx_task_start(download_id_);
  if (result < 0) {
    LOG(ERROR) << "ResumeRequest apx_task_start failed, result is : " << result;
    SendError();
  }

  is_paused_ = false;
}

void DownloadFileImplSpeed::CancelRequest() const {
  int result = apx_task_stop(download_id_);
  //result = apx_task_delete(download_id_);
  if (result < 0) {
  //  LOG(ERROR) << "CancelRequest apx_task_delete failed";
  }
}

int DownloadFileImplSpeed::UpdateDownloadMaxSpeed() const {
   if(!GetDownloadManager())
      return 0;
   else
      return GetDownloadManager()->GetDelegate()->GetDownloadMaxSpeed();
}

void DownloadFileImplSpeed::StreamActive() {
  base::TimeTicks start(base::TimeTicks::Now());
  base::TimeTicks now;
  base::TimeDelta delta(
    base::TimeDelta::FromMilliseconds(kMaxTimeBlockingFileThreadMs));
  bool is_finished = false;
  bool is_failed = false;

  do {
    // get the task state, update ui and finished to break
    int state = apx_task_state_get(download_id_);
    switch (state) {
      case APX_TASK_STATE_ACTIVE:
        {
          u32 down_speed = 0;
          u32 up_speed = 0;
          #if defined(OS_POSIX)
          if (kNetworkStatus != 0) {
            count_per_second_ = down_speed;
            break;
          }
          #endif
          int ret = apx_task_speed_get(download_id_, &down_speed, &up_speed);
          if (ret < 0) {
            //LOG(ERROR) << "StreamActive apx_task_speed_get failed";
            break;
          }
          u32 down_splimit = 0;
	  ret = apx_task_limit_get(download_id_,&down_splimit);
          if (ret < 0) {
            break;
          }
          if (down_splimit!=0 && down_speed > down_splimit*1024)
            down_speed = down_splimit*1024;

          u64 total_size = 0;
          u64 local_size = 0;
          u64 up_size = 0;
          ret = apx_task_file_size_get(download_id_, &total_size, &local_size, &up_size);
          if (ret < 0) {
            //LOG(ERROR) << "StreamActive apx_task_file_size_get failed";
            break;
          }

          // update the total size for new download
          if (bytes_total_ == 0) {
            bytes_total_ = total_size;
            BrowserThread::PostTask(
              BrowserThread::UI,
              FROM_HERE,
              base::Bind(&DownloadDestinationObserver::DestinationTotalSize,
                         observer_,
                         bytes_total_));
          }

          bytes_so_far_ = local_size;
          count_per_second_ = down_speed;
        }
        break;
      case APX_TASK_STATE_WAIT:
        break;
      case APX_TASK_STATE_FINISHED:
        {
          bytes_so_far_ = bytes_total_;
          is_finished = true;
          update_timer_.reset();
        }
        break;
      case APX_TASK_STATE_STOP:
        {
          if (!is_paused_) {
            bytes_so_far_ = bytes_total_;
            is_failed = true;
            update_timer_.reset();
          }
        }
        break;
      case APX_TASK_STATE_TERMINATED:
        {
          bytes_so_far_ = bytes_total_;
          is_failed = true;
          update_timer_.reset();
        }
        break;
      default:
        //NOTREACHED();
        break;
    }

    now = base::TimeTicks::Now();
    // wait for task finish and update the process
  } while (!is_finished && !is_failed && now - start <= delta);

  // If we're stopping to yield the thread, post a task so we come back.
  if (!is_finished && !is_failed && now - start > delta) {
    BrowserThread::PostDelayedTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&DownloadFileImplSpeed::StreamActive,
                   weak_factory_.GetWeakPtr()),
                   base::TimeDelta::FromMilliseconds(500));
  }

  // if failed then notify the observer failed
  if (is_failed) {
    // Signal successful completion and shut down processing.
    SendUpdate();

    std::unique_ptr<crypto::SecureHash> hash_state = NULL;
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&DownloadDestinationObserver::DestinationError,
                   observer_,
                   DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED,
                   bytes_so_far_,
                   base::Passed(&hash_state)));
  }

  // if finished then notify the observer completed
  if (is_finished) {
    // Signal successful completion and shut down processing.
    SendUpdate();

    // update the total size for new download
    if (bytes_total_ == 0) {
      int64_t total_size = 0;
	  if(target_path_.empty()) {
        char fpath[256]={0};
        char fname[256]={0};
        int ret = apx_task_file_name_get(download_id_, fpath, sizeof(fpath), fname, sizeof(fname));
        if (ret == 0) {
          #if defined(OS_WIN)
          base::FilePath file_path = base::FilePath(base::UTF8ToWide(fpath)).Append(base::UTF8ToWide(fname));
          #elif defined(OS_POSIX)
          base::FilePath file_path = base::FilePath(fpath).Append(fname);
          #endif
          target_path_ = file_path;
        }
      }
      if (base::GetFileSize(target_path_, &total_size)) {
        bytes_total_ = (u64)total_size;
        BrowserThread::PostTask(
            BrowserThread::UI,
            FROM_HERE,
            base::Bind(&DownloadDestinationObserver::DestinationTotalSize,
                       observer_,
                       bytes_total_));
        bytes_so_far_ = bytes_total_;
      }
    }

    std::unique_ptr<crypto::SecureHash> hash_state = NULL;
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&DownloadDestinationObserver::DestinationCompleted,
                   observer_,
                   bytes_so_far_,
                   base::Passed(&hash_state)));
  }
}

static std::string itos( int i )
{
   char ch[10];
   snprintf( ch, sizeof( ch ) - 1, "%d", i );
   std::string str( ch );
   return str;
}

void DownloadFileImplSpeed::ApxTaskCreate() {
  struct apx_trans_opt *trans_opt = NULL;
  struct apx_trans_stat *trans_stat = NULL;
  int is_exist = apx_task_exist(download_id_, &trans_opt, &trans_stat);

  // the new task should create a new one directly, if exist delete it first.
  // if it is breakpoint, we should check it exist first.
  // breakpoint-exist: get the task and resume it.
  // breakpoint-not-exist: create a new one.
  if ((!is_breakpoint_) || (is_breakpoint_ && is_exist < 0)) {
    // if the task exist, we should delete it first
    if (is_exist == 0) {
      int result = apx_task_remove(download_id_);
      if (result < 0) {
         LOG(ERROR) << "apx_task_delete failed, result is : " << result;
         SendError();
         return;
      }
    }

    #if defined(OS_WIN)
    unsigned int one_max_thread = 1;
    #else
    unsigned int one_max_thread = 5;
    #endif
    if (url_chain_.SchemeIsFtp())
       one_max_thread = 1;
    struct apx_trans_opt addnew_opt;
    memset(&addnew_opt, 0, sizeof(struct apx_trans_opt));
    addnew_opt.concurr = one_max_thread;

    addnew_opt.down_splimit = download_splimit_;
    addnew_opt.taskid = download_id_;

    std::string link_url = url_chain_.spec();
    addnew_opt.uri = new char[strlen(link_url.c_str())+5];
    if (url_chain_.SchemeIsBt()) {
      base::FilePath file_path;
      if (net::FileURLToFilePath(GURL(url_chain_), &file_path)) {
        #if defined(OS_WIN)
        strncpy(addnew_opt.uri, base::WideToUTF8(file_path.value()).c_str(),
        strlen(link_url.c_str())+5);
        #elif defined(OS_POSIX)
        strncpy(addnew_opt.uri, file_path.value().c_str(),
        strlen(link_url.c_str())+5);
        #endif
      }
    } else {
      strncpy(addnew_opt.uri, link_url.c_str(), strlen(link_url.c_str())+5);
    }

    std::string cookies = "Cookie: " + cookies_;
    addnew_opt.cookie = new char[5000];
    strncpy(addnew_opt.cookie, cookies.c_str(), 5000);

    std::string referrer = "Referer: " + referrer_url_.spec();
    addnew_opt.referer = new char[5000];
    strncpy(addnew_opt.referer, referrer.c_str(), 5000);

    addnew_opt.method = new char[5];
    strncpy(addnew_opt.method, method_.c_str(), 5);
    addnew_opt.pack_url = new char[5000];
    strncpy(addnew_opt.pack_url, pack_url_.c_str(), 5000);
    addnew_opt.type = APX_TASK_TYPE_DOWN;

    if(!url_chain_.SchemeIsBt() && !url_chain_.SchemeIsFtp()) { //bt not check(huk:some bug here about ftp, donnt know why  )
      int check_uri = apx_task_uri_check(&addnew_opt);
      if(check_uri < 0 ) {
        LOG(ERROR) << "apx_task_uri_check failed, check url is : " << check_uri;
        SendError();
        return;
      }
    }

    target_path_ = GetTargetTempPath();
    #if defined(OS_WIN)
    strncpy(addnew_opt.fname, base::WideToUTF8(target_path_.BaseName().value()).c_str(), sizeof(addnew_opt.fname)-1);
    strncpy(addnew_opt.fpath, base::WideToUTF8(target_path_.DirName().value()).c_str(), sizeof(addnew_opt.fpath)-1);
    if( apx_file_is_exist(addnew_opt.fpath,NULL) == 0 ) {
        if ( apx_file_mkdir( addnew_opt.fpath ) < 0 ) {
            SendError();
            return;
        }
    }
    #elif defined(OS_POSIX)
    strncpy(addnew_opt.fname, target_path_.BaseName().value().c_str(), sizeof(addnew_opt.fname)-1);
    strncpy(addnew_opt.fpath, target_path_.DirName().value().c_str(), sizeof(addnew_opt.fpath)-1);
    #endif

    int task_id  = apx_task_create(&addnew_opt);
    if( task_id < 0) {
      LOG(ERROR) << "apx_task_create failed, task id is : " << task_id;
      SendError();
      return;
    }

    // set ftp auth
    if (url_chain_.SchemeIsFtp()) {
      strncpy(addnew_opt.ftp_user, base::UTF16ToUTF8(auth_credential_.username()).c_str(), sizeof(addnew_opt.ftp_user) -1 );
      strncpy(addnew_opt.ftp_passwd, base::UTF16ToUTF8(auth_credential_.password()).c_str(), sizeof(addnew_opt.ftp_passwd) -1 );
      apx_task_ftp_account_set(task_id, addnew_opt.ftp_user, addnew_opt.ftp_passwd);
    }

    if(url_chain_.SchemeIsBt()) {
      struct btfile bt_file;
      apx_task_btfile_get(task_id,&bt_file);
      if(bt_file.size<=0) {
        SendError();
        return;
      }

      // update the bt file real name
      BrowserThread::PostTask(
          BrowserThread::UI,
          FROM_HERE,
          base::Bind(&DownloadDestinationObserver::DestinationUpdateFileName,
                     observer_,
                     std::string(bt_file.fn)));

      std::string bt_selected;
      for(int i=1;i<bt_file.size;i++) {
        bt_selected += itos(i);
        bt_selected += ",";
      }

      bt_selected += itos(bt_file.size);
      int select = apx_task_btfile_selected(task_id,const_cast<char*>(bt_selected.c_str()));
      if( select < 0) {
        LOG(ERROR) << "apx_task_btfile_selected failed, select is : " << select;
        SendError();
        return;
      }
    }

    task_status_ = true;
    DCHECK(task_id == (int)download_id_);
  } else {
    if (trans_opt) {
      #if defined(OS_WIN)
      target_path_ = base::FilePath(base::UTF8ToWide(std::string(trans_opt->fpath)));
      target_path_ = target_path_.Append(base::UTF8ToWide(std::string(trans_opt->fname)));
      #elif defined(OS_POSIX)
      target_path_ = base::FilePath(std::string(trans_opt->fpath));
      target_path_ = target_path_.Append(std::string(trans_opt->fname));
      #endif
    }

    task_status_ = true;
  }

  int nRet = apx_task_start(download_id_);
  if (nRet < 0) {
    LOG(ERROR) << "apx_task_start failed, task id is : " << download_id_ << ", ret is : " << nRet;
    SendError();
    return;
  }
}

std::string DownloadFileImplSpeed::GetFileName() {
  char filename[20]={0};
  srand((unsigned)time(0));
  for(int i = 0; i <= 14; i++) {
    filename[i] = rand()%10 + '0';
  }

  return std::string(filename);
}

base::FilePath DownloadFileImplSpeed::GetTargetTempPath() {
  base::FilePath temp_path;
  if (target_path_.empty()) {
    temp_path = default_download_directory_;
  } else {
    temp_path = target_path_.DirName();
  }
  #if defined(OS_WIN)
  temp_path = temp_path.Append(base::UTF8ToWide(GetFileName())).AddExtension(STRING_LITERAL("apxdownload"));
  #elif defined(OS_POSIX)
  temp_path = temp_path.Append(GetFileName()).AddExtension(std::string("apxdownload"));
  #endif
  return temp_path;
}

void DownloadFileImplSpeed::SendUpdate() {
  if (GetDownloadManager()->GetDelegate()->ShouldDownloadUpdate()) {
    if (task_status_) {
      BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&DownloadDestinationObserver::DestinationUpdate,
          observer_,
          bytes_so_far_,
          count_per_second_));
    }
  }
}

// Send an update on our progress.
void DownloadFileImplSpeed::SendError() {
  std::unique_ptr<crypto::SecureHash> hash_state = NULL;
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&DownloadDestinationObserver::DestinationError,
                 observer_,
                 DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED,
                 bytes_so_far_,
                 base::Passed(&hash_state)));
}

DownloadFileImplSpeed::RenameParameters::RenameParameters(
    RenameOption option,
    const base::FilePath& new_path,
    const RenameCompletionCallback& completion_callback)
    : option(option),
      new_path(new_path),
      retries_left(kMaxRenameRetries),
      completion_callback(completion_callback) {}

DownloadFileImplSpeed::RenameParameters::~RenameParameters() {}

}  // namespace content
