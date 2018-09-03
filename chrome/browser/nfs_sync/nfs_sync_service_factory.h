#ifndef CHROME_BROWSER_NFS_SYNC_NFS_SYNC_SERVICE_FACTORY_H_
#define CHROME_BROWSER_NFS_SYNC_NFS_SYNC_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class NfsSyncService;
class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

class NfsSyncServiceFactory : public BrowserContextKeyedServiceFactory {
public:
  static NfsSyncService* GetForBrowserContext(
      content::BrowserContext* browser_context);

  static NfsSyncService* GetForBrowserContextIfExists(
      content::BrowserContext* browser_context);

  static NfsSyncServiceFactory* GetInstance();

private:
  friend struct base::DefaultSingletonTraits<NfsSyncServiceFactory>;

  NfsSyncServiceFactory();
  ~NfsSyncServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(NfsSyncServiceFactory);
};

#endif  // CHROME_BROWSER_NFS_SYNC_NFS_SYNC_SERVICE_FACTORY_H_
