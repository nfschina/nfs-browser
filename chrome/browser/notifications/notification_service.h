#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_SERVICE_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_SERVICE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/url_request/url_fetcher_delegate.h"

class Profile;

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

class NotificationService : public KeyedService
                          , public net::URLFetcherDelegate {
public:
  explicit NotificationService(Profile* profile);
  ~NotificationService() override;

private:
  enum UpdateMode {
    AUTO_UPDATE,
    MANUAL_UPDATE,
  };

  void CheckForUpdate();

  // net::URLFetcherDelegate:
  // Called to process responses from the secure time service.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  bool HandleResponse();

  size_t max_response_size_;

  std::string url_;

  base::RepeatingTimer timer_;
  scoped_refptr<net::URLRequestContextGetter> getter_;
  std::unique_ptr<net::URLFetcher> time_fetcher_;

  base::ThreadChecker thread_checker_;

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NotificationService);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_SERVICE_H_
