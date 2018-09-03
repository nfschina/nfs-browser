#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_SERVICE_FACTORY_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_SERVICE_FACTORY_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class NotificationService;
class Profile;

// Singleton that owns all NotificationService and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated NotificationService.
class NotificationServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static NotificationService* GetForProfile(Profile* profile);

  static NotificationServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<NotificationServiceFactory>;

  NotificationServiceFactory();
  ~NotificationServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(NotificationServiceFactory);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_SERVICE_FACTORY_H_
