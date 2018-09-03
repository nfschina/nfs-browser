#include "chrome/browser/extensions/api/nfs_sync/nfs_sync_api.h"

#include "base/values.h"
#include "chrome/browser/nfs_sync/nfs_sync_service.h"
#include "chrome/browser/nfs_sync/nfs_sync_service_factory.h"
#include "chrome/common/extensions/api/nfs_sync.h"

namespace extensions {

#if defined(OS_POSIX)
const char* kErrorList[] = {
  "参数获取失败",
  "参数类型错误",
  "后台服务构造失败"
};
#elif defined(OS_WIN)
const wchar_t* kErrorList[] = {
  L"参数获取失败",
  L"参数类型错误",
  L"后台服务构造失败"
};
#endif

NfsSyncNfsUserLoginFunction::NfsSyncNfsUserLoginFunction() {
}

NfsSyncNfsUserLoginFunction::~NfsSyncNfsUserLoginFunction() {
}

AsyncExtensionFunction::ResponseAction NfsSyncNfsUserLoginFunction::Run() {
  std::unique_ptr<api::nfs_sync::NfsUserLogin::Params> params(
      api::nfs_sync::NfsUserLogin::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());

  std::unique_ptr<base::DictionaryValue> info(params->info.ToValue());
  DCHECK(info && info.get());
  if (!info || !info.get()) {
    result->SetInteger("error_code", 0);
    result->SetString("error_msg", kErrorList[0]);
    return RespondNow(OneArgument(std::move(result)));
  }

  std::string name, id;
  if (!info->HasKey("name") ||
      !info->GetString("name", &name) ||
      !info->HasKey("id") ||
      !info->GetString("id", &id)) {
    result->SetInteger("error_code", 1);
    result->SetString("error_msg", kErrorList[1]);
    return RespondNow(OneArgument(std::move(result)));
  }

  std::string email;
  if (info->HasKey("email")) {
    if (!info->GetString("email", &email)) {
      result->SetInteger("error_code", 1);
      result->SetString("error_msg", kErrorList[1]);
      return RespondNow(OneArgument(std::move(result)));
    }
  }

  NfsSyncService* nfs_sync_service =
      NfsSyncServiceFactory::GetForBrowserContext(browser_context());
  DCHECK(nfs_sync_service);
  if (!nfs_sync_service) {
    result->SetInteger("error_code", 2);
    result->SetString("error_msg", kErrorList[2]);
    return RespondNow(OneArgument(std::move(result)));
  }

  nfs_sync_service->Login(name, email, id);

  return RespondNow(NoArguments());
}

NfsSyncIsLoginFunction::NfsSyncIsLoginFunction() {
}

NfsSyncIsLoginFunction::~NfsSyncIsLoginFunction() {
}

AsyncExtensionFunction::ResponseAction NfsSyncIsLoginFunction::Run() {
  NfsSyncService* nfs_sync_service =
      NfsSyncServiceFactory::GetForBrowserContext(browser_context());
  DCHECK(nfs_sync_service);
  if (!nfs_sync_service) {
    std::unique_ptr<base::FundamentalValue> value(
        new base::FundamentalValue(false));
    return RespondNow(OneArgument(std::move(value)));
  }

  if (nfs_sync_service->IsLoggedIn()) {
    std::unique_ptr<base::FundamentalValue> value(
        new base::FundamentalValue(true));
    return RespondNow(OneArgument(std::move(value)));
  }

  std::unique_ptr<base::FundamentalValue> value(
      new base::FundamentalValue(false));
  return RespondNow(OneArgument(std::move(value)));
}

}
