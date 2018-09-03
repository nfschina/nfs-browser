#include "chrome/browser/nfs_sync/nfs_sync_service_factory.h"

#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/nfs_sync/nfs_sync_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/nfs_sync/nfs_sync_pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context_getter.h"

std::unique_ptr<NfsSyncService> CreateNfsSyncService(
    bool logged_in, net::URLRequestContextGetter* getter, Profile* profile) {
  std::unique_ptr<NfsSyncService> nfs_sync_service(
      new NfsSyncService(logged_in, getter, profile));
  return nfs_sync_service;
}

// static
NfsSyncService* NfsSyncServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<NfsSyncService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
NfsSyncService* NfsSyncServiceFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return static_cast<NfsSyncService*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

// static
NfsSyncServiceFactory* NfsSyncServiceFactory::GetInstance() {
  return base::Singleton<NfsSyncServiceFactory>::get();
}

NfsSyncServiceFactory::NfsSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "nfsSyncService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(BookmarkModelFactory::GetInstance());
}

NfsSyncServiceFactory::~NfsSyncServiceFactory() {
}

KeyedService* NfsSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  net::URLRequestContextGetter* getter =
      g_browser_process->system_request_context();
  DCHECK(getter);
  if (!getter) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }

  PrefService* pref_service = profile->GetPrefs();
  DCHECK(pref_service);
  if (!pref_service) {
    return nullptr;
  }

  std::unique_ptr<NfsSyncService> nfs_sync_service =
      CreateNfsSyncService(pref_service->GetBoolean(nfs_sync::prefs::kIsLogin),
                            getter,
                            profile);
  if (!nfs_sync_service->Init()) {
    return nullptr;
  }

  return nfs_sync_service.release();
}

content::BrowserContext* NfsSyncServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

void NfsSyncServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      nfs_sync::prefs::kNfsSyncStatus,
      0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterUint64Pref(
      nfs_sync::prefs::kNfsSyncTime,
      0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      nfs_sync::prefs::kIsFirstLogin, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      nfs_sync::prefs::kIsChanged, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      nfs_sync::prefs::kIsLogin, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(nfs_sync::prefs::kNfsAccountInfo);
  registry->RegisterDictionaryPref(nfs_sync::prefs::kMotifyTime);
}

bool NfsSyncServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool NfsSyncServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
