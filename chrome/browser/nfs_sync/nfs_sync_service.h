#ifndef CHROME_BROWSER_NFS_SYNC_NFS_SYNC_SERVICE_H_
#define CHROME_BROWSER_NFS_SYNC_NFS_SYNC_SERVICE_H_

#include "base/debug/leak_tracker.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/nfs_sync/account_info.h"
#include "components/nfs_sync/nfs_data_syncer.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;
class Profile;

namespace base {
class ListValue;
class SingleThreadTaskRunner;
class Thread;
class Value;
}

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}

namespace nfs_sync {
class AccountChecker;
class NfsSyncObserver;
class NfsThemeHandle;
}

namespace password_manager {
class PasswordStore;
}

namespace net {
class URLFetcher;
class URLRequestContext;
class URLRequestContextGetter;
}

class NfsSyncService : public KeyedService
                      , public bookmarks::BookmarkModelObserver
                      , public net::URLFetcherDelegate {
public:
  explicit NfsSyncService(bool logged_in,
                           net::URLRequestContextGetter* getter,
                           Profile* profile);
  ~NfsSyncService() override;

  bool Init();

  void AddObserver(nfs_sync::NfsSyncObserver* observer);
  void RemoveObserver(nfs_sync::NfsSyncObserver* observer);

  void ReportLoginSuccess(const std::string& name,
                          const std::string& email,
                          const std::string& id);
  void ReportLogoutSuccess();

  bool IsLoggedIn() { return logged_in_; }
  nfs_sync::AccountInfo GetAccountInfo() const { return account_info_; }
  void SetAccountInfo(nfs_sync::AccountInfo info);

  void Login(const std::string& name,
             const std::string& email,
             const std::string& user_id);

  void PostThemeToServer(const std::string& theme_id);

  void PostPasswordToServer(const std::string& password);

  void SyncBookmarkNow();
  void SyncThemeNow();
  void SyncPasswordNow();

private:
  // net::URLFetcherDelegate:
  // Called to process responses from the secure time service.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // bookmarks::BookmarkModelObserver:
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void OnWillRemoveBookmarks(bookmarks::BookmarkModel* model,
                             const bookmarks::BookmarkNode* parent,
                             int old_index,
                             const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           int old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& no_longer_bookmarked) override;
  void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model,
                                   const std::set<GURL>& removed_urls) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                               const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;

  void LoadFromPrefs();
  void SaveToPrefs(const std::string& name,
                   const std::string& email,
                   const std::string& user_id);
  void ClearToPrefs();

  void PostAllDataToServer();

  void ApplyBookmarkData(const bookmarks::BookmarkNode* node,
                         base::ListValue* value);
  void MergeBookmarks(base::ListValue* server_data);

  void OnCheckAccount(const bool result);
  void OnGetThemeRequest(const std::string& response);
  void OnGetBookmarkData(const std::string& bookmark_data);
  void OnPostPasswordRequest(const std::string& password_data);
  void OnGetPasswordRequest(const std::string& password_data);
  void AddLoginDB(const std::string& url, const std::string& name, const std::string& password);
  void PostAllPasswordsToServer();
  void ResetLoginDB();

  void Cleanup();

  bool logged_in_;

  bool bookmark_is_motify_;

  // The thread used by the history service to run complicated operations.
  // |thread_| is null once Cleanup() is called.
  base::Thread* thread_;

  std::unique_ptr<net::URLFetcher> fetcher_;
  std::unique_ptr<nfs_sync::AccountChecker> account_checker_;
  std::unique_ptr<nfs_sync::NfsThemeFetcher> nfs_theme_fetcher_;
  std::unique_ptr<nfs_sync::NfsBookmarkFetcher> nfs_bookmark_fetcher_;
  std::unique_ptr<nfs_sync::NfsPasswordFetcher> nfs_password_fetecher_;

  std::unique_ptr<nfs_sync::NfsThemeHandle> nfs_theme_handler_;

  Profile* profile_;

  PrefService* pref_service_;

  bookmarks::BookmarkModel* bookmark_model_;

  scoped_refptr<net::URLRequestContextGetter> getter_;

  // The observers.
  base::ObserverList<nfs_sync::NfsSyncObserver> observers_;

  nfs_sync::AccountInfo account_info_;

  DISALLOW_COPY_AND_ASSIGN(NfsSyncService);
};  // class NfsSyncService

#endif  // CHROME_BROWSER_NFS_SYNC_NFS_SYNC_SERVICE_H_
