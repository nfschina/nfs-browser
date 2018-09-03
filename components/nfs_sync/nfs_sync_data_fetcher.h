#ifndef COMPONENTS_NFS_SYNC_NFS_SYNC_DATA_FETCHER_H_
#define COMPONENTS_NFS_SYNC_NFS_SYNC_DATA_FETCHER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace nfs_sync {

class NfsSyncDataFetcher : public net::URLFetcherDelegate {
public:

private:
  std::unique_ptr<net::URLFetcher> fetcher_;
  scoped_refptr<net::URLRequestContextGetter> getter_;

  DISALLOW_COPY_AND_ASSIGN(NfsSyncDataFetcher);
};

}  // namespace nfs_sync

#endif  // COMPONENTS_NFS_SYNC_NFS_SYNC_DATA_FETCHER_H_
