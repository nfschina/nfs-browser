#include "components/nfs_sync/account_checker.h"

#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/nfs_sync/nfs_sync_utils.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

const char kURL[] = "http://124.16.141.197:10001/account";

}

namespace nfs_sync {

AccountChecker::AccountChecker(net::URLRequestContextGetter* getter)
    : getter_(getter) {
  DCHECK(getter_);
}

AccountChecker::~AccountChecker() {}

void AccountChecker::CheckAccount(
    AccountInfo account_info, AccountCheckCallback callback) {
  callback_ = callback;

  std::string url = base::StringPrintf("%s%s%s&%s%s&%s%s", kURL,
                                       "?user_id=", account_info.id.c_str(),
                                       "name=", account_info.name.c_str(),
                                       "email=", account_info.email.c_str());

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

void AccountChecker::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  if (fetcher_->GetStatus().status() != net::URLRequestStatus::SUCCESS ||
      fetcher_->GetResponseCode() != 200) {
    return;
  }

  std::string response_body;
  if (!fetcher_->GetResponseAsString(&response_body)) {
    return;
  }

  std::unique_ptr<base::DictionaryValue> value =
      JSONStringToDictionary(response_body);
  if (!value || !value.get()) {
    return;
  }

  int code = 0;
  value->GetInteger("code", &code);
  if (0 == code) {
    callback_.Run(false);
  } else if (code > 0) {
    callback_.Run(true);
  } else {
    LOG(ERROR) << "ERROR";
  }
}

}  // namespace nfs_sync
