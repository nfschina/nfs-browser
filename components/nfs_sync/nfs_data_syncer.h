#ifndef COMPONENTS_NFS_SYNC_NFS_DATA_SYNCER_H_
#define COMPONENTS_NFS_SYNC_NFS_DATA_SYNCER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLRequestContextGetter;
}

namespace nfs_sync {

class NfsThemeFetcher : public net::URLFetcherDelegate {
public:
  typedef base::Callback<void(const std::string&)> PostResultCallback;

  explicit NfsThemeFetcher(net::URLRequestContextGetter* getter);
  ~NfsThemeFetcher() override;

  void GetThemeFromServer(
      const std::string& user_id, PostResultCallback callback);
  void PostThemeToServer(
      const std::string& user_id,
      const std::string& theme_id,
      PostResultCallback callback);

private:
  // net::URLFetcherDelegate:
  // Called to process responses from the secure time service.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  PostResultCallback callback_;

  std::unique_ptr<net::URLFetcher> fetcher_;
  scoped_refptr<net::URLRequestContextGetter> getter_;

  DISALLOW_COPY_AND_ASSIGN(NfsThemeFetcher);
};  // class NfsThemeFetcher

class NfsPasswordFetcher : public net::URLFetcherDelegate {
public:
  typedef base::Callback<void(const std::string&)> PostResultCallback;

  explicit NfsPasswordFetcher(net::URLRequestContextGetter* getter);
  ~NfsPasswordFetcher() override;

  void GetPasswordFromServer(
      const std::string& user_id, PostResultCallback callback);
  void PostPasswordToServer(
      const std::string& password_list,
      PostResultCallback callback);

private:
  // net::URLFetcherDelegate:
  // Called to process responses from the secure time service.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  PostResultCallback callback_;

  std::unique_ptr<net::URLFetcher> fetcher_;
  scoped_refptr<net::URLRequestContextGetter> getter_;

  DISALLOW_COPY_AND_ASSIGN(NfsPasswordFetcher);
};  // class NfsThemeFetcher

class NfsBookmarkFetcher : public net::URLFetcherDelegate {
public:
  typedef base::Callback<void(const std::string&)> PostResultCallback;

  explicit NfsBookmarkFetcher(net::URLRequestContextGetter* getter);
  ~NfsBookmarkFetcher() override;

  void GetBookmarkFromServer(
      const std::string& user_id, PostResultCallback callback);
  void PostBookmarkToServer(
      const std::string& user_id,
      const std::string& bookmark_data,
      PostResultCallback callback);

private:
  // net::URLFetcherDelegate:
  // Called to process responses from the secure time service.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  PostResultCallback callback_;

  std::unique_ptr<net::URLFetcher> fetcher_;
  scoped_refptr<net::URLRequestContextGetter> getter_;

  DISALLOW_COPY_AND_ASSIGN(NfsBookmarkFetcher);
};  // class NfsBookmarkFetcher

}  // namespace nfs_sync

#endif  // COMPONENTS_NFS_SYNC_NFS_DATA_SYNCER_H_
