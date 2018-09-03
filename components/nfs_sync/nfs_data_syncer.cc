#include "components/nfs_sync/nfs_data_syncer.h"

#include "base/strings/stringprintf.h"
#include "components/nfs_sync/nfs_sync_utils.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

const char kThemeURL[] = "http://124.16.141.197:10001/theme";
const char kBookmarkURL[] = "http://124.16.141.197:10001/bookmark";
const char kPasswordURL[] = "http://124.16.141.197:10001/password";
const char kContentTypeJSON[] = "application/json";
const char kContentHeadersJSON[] =
    "Content-Type: application/json; charset=utf-8";

}

namespace nfs_sync {

//////////////////////// NfsThemeFetcher ////////////////////////

NfsThemeFetcher::NfsThemeFetcher(net::URLRequestContextGetter* getter)
    : getter_(getter) {
  DCHECK(getter_);
}

NfsThemeFetcher::~NfsThemeFetcher() {}

void NfsThemeFetcher::GetThemeFromServer(
    const std::string& user_id, PostResultCallback callback) {
  callback_ = callback;

  std::string url = base::StringPrintf("%s%s%s&%s",
                                       kThemeURL,
                                       "?user_id=",
                                       user_id.c_str(),
                                       "type=get");

  fetcher_ = net::URLFetcher::Create(GURL(url), net::URLFetcher::GET, this);
  if (!fetcher_ || !fetcher_.get()) {
    return;
  }

  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());

  fetcher_->Start();
}

void NfsThemeFetcher::PostThemeToServer(const std::string& user_id,
                                         const std::string& theme_id,
                                         PostResultCallback callback) {
  callback_ = callback;

  std::string url = base::StringPrintf("%s%s%s&%s%s&%s",
                                       kThemeURL,
                                       "?user_id=",
                                       user_id.c_str(),
                                       "theme_id=",
                                       theme_id.c_str(),
                                       "type=post");

  fetcher_ = net::URLFetcher::Create(GURL(url), net::URLFetcher::GET, this);
  if (!fetcher_ || !fetcher_.get()) {
    return;
  }

  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());

  fetcher_->Start();
}

void NfsThemeFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  if (fetcher_->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      fetcher_->GetResponseCode() != 200) {
    return;
  }

  std::string response_body;
  if (!fetcher_->GetResponseAsString(&response_body)) {
    return;
  }

  callback_.Run(response_body);
}

//////////////////////// NfsPasswordFetcher ////////////////////////

NfsPasswordFetcher::NfsPasswordFetcher(net::URLRequestContextGetter* getter)
    : getter_(getter) {
  DCHECK(getter_);
}

NfsPasswordFetcher::~NfsPasswordFetcher() {}

void NfsPasswordFetcher::GetPasswordFromServer(
    const std::string& user_id, PostResultCallback callback) {
  callback_ = callback;

  std::string url = base::StringPrintf("%s%s%s",
                                       kPasswordURL,
                                       "?user_id=",
                                       user_id.c_str());

  fetcher_ = net::URLFetcher::Create(GURL(url), net::URLFetcher::GET, this);
  if (!fetcher_ || !fetcher_.get()) {
    return;
  }

  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());

  fetcher_->Start();
}

void NfsPasswordFetcher::PostPasswordToServer(const std::string& password_list,
                                         PostResultCallback callback) {
  callback_ = callback;
  fetcher_ =
      net::URLFetcher::Create(GURL(kPasswordURL), net::URLFetcher::POST, this);
  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                         net::LOAD_BYPASS_CACHE);
  fetcher_->SetUploadData(kContentTypeJSON, password_list);
  fetcher_->SetExtraRequestHeaders(kContentHeadersJSON);
  fetcher_->Start();
}

void NfsPasswordFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  if (fetcher_->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      fetcher_->GetResponseCode() != 200) {
    return;
  }

  std::string response_body;

  if (!fetcher_->GetResponseAsString(&response_body)) {
    return;
  }
  callback_.Run(response_body);
}


//////////////////////// NfsBookmarkFetcher ////////////////////////

NfsBookmarkFetcher::NfsBookmarkFetcher(net::URLRequestContextGetter* getter)
    : getter_(getter) {
  DCHECK(getter_);
}

NfsBookmarkFetcher::~NfsBookmarkFetcher() {}

void NfsBookmarkFetcher::GetBookmarkFromServer(
    const std::string& user_id, PostResultCallback callback) {
  callback_ = callback;

  std::string url = base::StringPrintf("%s%s%s&%s",
                                       kBookmarkURL,
                                       "?user_id=",
                                       user_id.c_str(),
                                       "type=get");

  fetcher_ = net::URLFetcher::Create(GURL(url), net::URLFetcher::GET, this);
  if (!fetcher_ || !fetcher_.get()) {
    return;
  }

  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());

  fetcher_->Start();
}

void NfsBookmarkFetcher::PostBookmarkToServer(
    const std::string& user_id,
    const std::string& bookmark_data,
    PostResultCallback callback) {
  callback_ = callback;

  if (bookmark_data.empty()) {
    return;
  }
  std::string encrypted;
  if (!nfs_sync::EncryptAndBase64(bookmark_data, &encrypted)) {
    return;
  }

  std::string url = base::StringPrintf("%s%s%s",
                                       kBookmarkURL,
                                       "?user_id=",
                                       user_id.c_str());
  fetcher_ =
      net::URLFetcher::Create(GURL(url), net::URLFetcher::POST, this);
  fetcher_->SaveResponseWithWriter(
      std::unique_ptr<net::URLFetcherStringWriter>(
          new net::URLFetcherStringWriter));
  fetcher_->SetRequestContext(getter_.get());
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                         net::LOAD_BYPASS_CACHE);
  fetcher_->SetUploadData(kContentTypeJSON, encrypted);
  fetcher_->SetExtraRequestHeaders(kContentHeadersJSON);
  fetcher_->Start();
}

void NfsBookmarkFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  if (fetcher_->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      fetcher_->GetResponseCode() != 200) {
    return;
  }

  std::string response_body;
  if (!fetcher_->GetResponseAsString(&response_body)) {
    return;
  }

  callback_.Run(response_body);
}

}  // namespace nfs_sync
