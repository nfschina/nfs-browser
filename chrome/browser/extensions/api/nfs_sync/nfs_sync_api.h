#ifndef CHROME_BROWSER_EXTENSIONS_API_NFS_SYNC_NFS_SYNC_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_NFS_SYNC_NFS_SYNC_API_H_

#include "base/macros.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class NfsSyncNfsUserLoginFunction : public UIThreadExtensionFunction {
public:
  DECLARE_EXTENSION_FUNCTION("nfs.sync.nfsUserLogin",
                             NFS_SYNC_NFSUSERLOGIN)
  NfsSyncNfsUserLoginFunction();
  AsyncExtensionFunction::ResponseAction Run() override;

private:
  ~NfsSyncNfsUserLoginFunction() override;

  DISALLOW_COPY_AND_ASSIGN(NfsSyncNfsUserLoginFunction);
};

class NfsSyncIsLoginFunction : public UIThreadExtensionFunction {
public:
  DECLARE_EXTENSION_FUNCTION("nfs.sync.isLogin",
                             NFS_SYNC_ISLOGIN)
  NfsSyncIsLoginFunction();
  AsyncExtensionFunction::ResponseAction Run() override;

private:
  ~NfsSyncIsLoginFunction() override;

  DISALLOW_COPY_AND_ASSIGN(NfsSyncIsLoginFunction);
};

}

#endif  // CHROME_BROWSER_EXTENSIONS_API_NFS_SYNC_NFS_SYNC_API_H_
