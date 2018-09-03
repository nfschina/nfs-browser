#pragma once
#include "chrome/common/extensions/api/checkmurl.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include <map>
#include <string.h>
#include <vector>
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/non_thread_safe.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/geometry/size.h"
#include "base/threading/thread_checker.h"

#include "third_party/c-vtapi/lib/VtUrl.h"
#include "third_party/c-vtapi/lib/VtResponse.h"
#include "third_party/c-vtapi/lib/cJSON.h"
#include "third_party/jansson/src/jansson.h"

#include "third_party/sqlite/amalgamation/sqlite3.h"
typedef base::Callback<void(int)> Responsecallback;


namespace extensions {
  class CheckmurlCheckurlFunction: public ChromeAsyncExtensionFunction {
    public:
      DECLARE_EXTENSION_FUNCTION("checkmurl.checkurl",CHECKMURL_CHECKURL);
      CheckmurlCheckurlFunction();
      bool RunAsync() override;
      char *urls;
      void SendResult(int result);
    protected:
      ~CheckmurlCheckurlFunction() override{};
  };
}





class CheckurlManager :  public base::NonThreadSafe {
  public:
    CheckurlManager();
    ~CheckurlManager() ;
    sqlite3* checkdb;
    int CheckUrlTask(char*p, Responsecallback callback,CheckurlManager *,const char*);//method func
    static int dbinquery(CheckurlManager*,const char*,const char *,int * );
    static int urlIfcomplete(const char *);
    static int dbinsert(CheckurlManager* ,const char *);
    static int dbcallback(void *data, int argc, char **argv, char **azColName);
    const static int virusint;
    static int strprocess(char*,char**);
    std::map <std::string,int>enumMap;
  private:
    //Used to post tasks to checkurl thread
    scoped_refptr<base::SingleThreadTaskRunner> check_url_task_runner_;

    static char* checkapi(char* apikey,char* url);
          static char* jsonFinParse(char* stringjson);
    static char* parseJsonRes(cJSON* psub,int cnt);
    static cJSON* parseJson(char* pMsg);
    static int geturlrst(char* rul,CheckurlManager *,const char*);
    //Used to ensure CheckurlManager methods are called on the same thread.
    base::ThreadChecker thread_checker_;
    //Used to checkurl
    std::unique_ptr<base::Thread> check_url_thread_;

    Responsecallback callback_;

    //Used to weakly bind |this| to methods
    //Note:Weak pointers must be invalidated before all other member variables.
    base::WeakPtrFactory<CheckurlManager> weak_factory_;
};
