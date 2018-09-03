#include "chrome/browser/notifications/notification_service_factory.h"

#include "chrome/browser/notifications/notification_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "net/url_request/url_request_context_getter.h"

// static
NotificationService* NotificationServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<NotificationService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
NotificationServiceFactory* NotificationServiceFactory::GetInstance() {
  return base::Singleton<NotificationServiceFactory>::get();
}

NotificationServiceFactory::NotificationServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "NotificationService",
        BrowserContextDependencyManager::GetInstance()) {
}

NotificationServiceFactory::~NotificationServiceFactory() {
}

KeyedService* NotificationServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  // std::unique_ptr<NotificationService> notification_service(
  //     new NotificationService(
  //         static_cast<Profile*>(profile)->GetRequestContext()));
  // return notification_service.release();
  return new NotificationService(static_cast<Profile*>(profile));
}

content::BrowserContext* NotificationServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
