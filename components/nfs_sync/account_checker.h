#ifndef COMPONENTS_NFS_SYNC_ACCOUNT_CHECKER_H_
#define COMPONENTS_NFS_SYNC_ACCOUNT_CHECKER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/nfs_sync/account_info.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace nfs_sync {

class AccountChecker : public net::URLFetcherDelegate {
public:
  typedef base::Callback<void(const bool)> AccountCheckCallback;

  explicit AccountChecker(net::URLRequestContextGetter* getter);
  ~AccountChecker() override;

  void CheckAccount(AccountInfo account_info,
                    AccountCheckCallback callback);

private:
  // net::URLFetcherDelegate:
  // Called to process responses from the secure time service.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  AccountCheckCallback callback_;

  std::unique_ptr<net::URLFetcher> fetcher_;

  scoped_refptr<net::URLRequestContextGetter> getter_;

  DISALLOW_COPY_AND_ASSIGN(AccountChecker);
};  // class AccountChecker

}  // namespace nfs_sync

#endif  // COMPONENTS_NFS_SYNC_ACCOUNT_CHECKER_H_
