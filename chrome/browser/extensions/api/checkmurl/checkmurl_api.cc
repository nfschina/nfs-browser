#include <map>
#if defined(OS_LINUX)
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <string.h>
#include "checkmurl_api.h"
#include "chrome/common/extensions/api/checkmurl.h"
#include "third_party/c-vtapi/lib/VtUrl.h"
#include "third_party/c-vtapi/lib/VtResponse.h"
#include "third_party/c-vtapi/lib/cJSON.h"
#include "third_party/jansson/src/jansson.h"
#include "base/debug/stack_trace.h"
#include "base/logging.h"//add by wangn
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/profile_helper.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "third_party/sqlite/amalgamation/sqlite3.h"//add by wangn
#include "base/task_runner_util.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_view_host.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/browser_thread_impl.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/pre_install/preinstall_crx_info.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"

#include "third_party/c-vtapi/lib/VtUrl.h"
#include "third_party/c-vtapi/lib/VtResponse.h"
#include "third_party/c-vtapi/lib/cJSON.h"
#include "third_party/jansson/src/jansson.h"
using content::BrowserThread;

namespace {
  const char kCheckUrlThreadName[] = "Chrome_CheckUrlThread";
}

cJSON * CheckurlManager::parseJson(char * pMsg) {
  if(NULL == pMsg) {
    printf("pMsg is NULL\n");
  //exit(1);
  }
  #if defined(OS_WIN)
  return NULL;
  #else
  cJSON * pJson = cJSON_Parse(pMsg);
  if(NULL == pJson) {
    printf("pJson is NULL\n");
  //exit(1);
  }
  // get string from json
  cJSON * pSub = cJSON_GetObjectItem(pJson, "scans");
  if(NULL == pSub) {
    printf("There is no scans contents in json\n");
  //exit(1);
  }
  return pSub;
  #endif
}


char *CheckurlManager::parseJsonRes(cJSON *psub,int cnt) {
  char *res = NULL;
  res = (char* )malloc(1024* sizeof(char));
  if (NULL == res) {
    LOG(ERROR)<< "res is NULL";
    exit(1);
  }

  #if !defined(OS_WIN)
  if(psub == NULL) {
    sprintf(res,"%s","No Virus");
    cJSON_Delete(psub);
  }
  else {
    while (psub->child ) {
      int count = 0;
      cJSON *pSubchildres = psub->child->child;

      while (pSubchildres && strcasecmp(pSubchildres->string,"result"))
        pSubchildres = pSubchildres->next;

      if(strcasecmp(pSubchildres->valuestring,"clean site") && strcasecmp(pSubchildres->valuestring,"unrated site")) {//not equal to "clean site" or "unrated iste"
        if(!strcasecmp(psub->child->string,"Kaspersky") || !strcasecmp(psub->child->string,"Rising") || !strcasecmp(psub->child->string,"Tencent")) {
          cnt = cnt +1;
          sprintf(res,"%s",pSubchildres->valuestring);
        }
        else if(!strcasecmp(psub->child->string,"Google Safebrowsing") || !strcasecmp(psub->child->string,"G-Data")){
          if(count == 2) {
            cnt = cnt +1;
            sprintf(res,"%s",pSubchildres->valuestring);
          }
          count = count + 1;
        }
      }
      psub->child = psub->child->next;

      if(cnt == 0) {
        memset(res,0,1024 *sizeof(char));
        sprintf(res,"%s","No Virus");
        cJSON_Delete(pSubchildres);
      }
      else
        cJSON_Delete(pSubchildres);
    }
  }
  #endif
  return res;
}

char* CheckurlManager::jsonFinParse(char *stringjson) {
  int count = 0;
  cJSON *sJson = parseJson(stringjson);
  if(NULL == sJson) {
    char *resultjson = (char *)malloc(1024 *sizeof(char));
    strcpy(resultjson,"there is no scans contents in Json");
  }
  char *resultjson = parseJsonRes(sJson,count);
  return resultjson;
}


char* CheckurlManager::checkapi(char *apikey,char *url) {
  LOG(INFO) << "checkapi";
  #if !defined(OS_WIN)
  struct VtUrl *file_scan;
  struct VtResponse *response;
  int ret = 0;
  bool all_info = false;
  bool report_scan = false;
  char *str = NULL;
  char *resultstr = (char* )malloc(1024* sizeof(char));
  file_scan = VtUrl_new();
  VtUrl_setApiKey(file_scan, apikey);
  if (!apikey) {
    printf("Must set --apikey first\n");
    exit(1);
  }
  ret = VtUrl_report(file_scan, url, all_info, report_scan);
  memset(resultstr,0,1024 *sizeof(char));
  if (ret) {
    if(ret > 0) {
      printf("curl_easy_perform() failed123..Check network connection please!! ret is %d\n",ret);
      strcpy(resultstr,"can't access virustotal server");
      printf("resu:%s\n",resultstr);
    }
    else if(ret < 0) {
      printf("virustotal post issue : one public api key limitation: 4 quests per minute!! ret is %d \n", ret);
      strcpy(resultstr,"virustotal post issue");
    }
  }
  else {
    response = VtUrl_getResponse(file_scan);
    str = VtResponse_toJSONstr(response, VT_JSON_FLAG_INDENT);
    if (str) {
      char *rs = jsonFinParse(str);//scan results
      //printf("checkurl_result:%s\n",rs);
      strcpy(resultstr,rs);
      free(str);
    }
    VtResponse_put(&response);
  }
  VtUrl_put(&file_scan);
  return resultstr;
  #endif
  return NULL;
}

int CheckurlManager::geturlrst(char* rul,CheckurlManager *imt,const char*domurl){
  int CheckUrlResult = 0;
  LOG(INFO) << "Checkurl:geturlrst\n";

  char *apikeystr =(char* )malloc(1024* sizeof(char));
  strcpy(apikeystr,"4e30c69abbb5efed4df5f479e2e7462682d99cd562dfa77897bd497fd4f3bcbc");
  char *rststring = checkapi(apikeystr,rul);
  if(strstr(rststring,"No Virus") || !strcmp(rststring,"") || strstr(rststring,"can't access virustotal server") || strstr(rststring,"virustotal post issue"))
    CheckUrlResult = 0;
  else
    CheckUrlResult = 1;

  imt->enumMap.insert(std::make_pair(domurl,CheckUrlResult));

  free(apikeystr);
  free(rststring);

  //LOG(ERROR) << "CHECKURLRESULT:"<<CheckUrlResult;
  return CheckUrlResult;
}




int CheckurlManager::CheckUrlTask(char *url,  Responsecallback callback,CheckurlManager *imt,const char *domurl){
  callback_ = callback;
  //pid_t tid1 = syscall(SYS_gettid);
  check_url_task_runner_ = check_url_thread_->task_runner();
  PostTaskAndReplyWithResult(check_url_task_runner_.get(),FROM_HERE,base::Bind(&CheckurlManager::geturlrst,url,imt,domurl),callback_);

  return 0;
}

CheckurlManager::CheckurlManager()
	: check_url_thread_ (
		new base::Thread(kCheckUrlThreadName)),
	weak_factory_(this) {
  LOG(INFO) << "CheckurlManager() Start\n";
  DCHECK(thread_checker_.CalledOnValidThread());
  if(!check_url_thread_->Start()) {
    LOG(INFO) << "########################Cannot start CheckurlManager\n";
  } else {
    LOG(INFO) << "########################Start CheckurlManager\n";
  }
}

CheckurlManager::~CheckurlManager(){
  LOG(INFO) << "########################CheckurlManager() End\n";
  check_url_thread_->StopSoon();
}



int CheckurlManager::dbcallback(void *data, int argc, char **argv, char **azColName) {
  int i;
  fprintf(stderr, "%s: ", (const char*)data);
  LOG(INFO) << "dbcallback:argc:" << argc <<"\n";
  for(i=0; i<argc; i++) {
    printf("***dbcallback %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  return 0;
}


int CheckurlManager::dbinsert(CheckurlManager* imanager,const char *url) {
  char tmp[1024]={0};
  char *sql = tmp;
  int rc;
  sprintf(sql,"INSERT INTO VIRUS(SCAN_URL) VALUES('%s');",url);
  char* zErrMsg = 0;

  rc = sqlite3_exec(imanager->checkdb,sql,imanager->dbcallback,0,&zErrMsg);
  if(rc != SQLITE_OK) {
    fprintf(stderr,"SQL error: %s\n",zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
    fprintf(stdout,"insert Operation done successfully\n");
  }

  return 0;
}

int CheckurlManager::urlIfcomplete(const char *url) {
  int urlcomplete = 0;
  if(!strstr("http://",url)) {
    urlcomplete = 1;
  } else if(!strstr("https://",url)) {
    urlcomplete = 2;
  } else {
    urlcomplete = 0;
  }

  return urlcomplete;
}


int CheckurlManager::strprocess(char *mid,char **last) {
  int trimlen = strlen(mid);
  if(mid[trimlen-1]=='/') {
    strncpy(*last,mid,trimlen-1);
  } else {
    strncpy(*last,mid,trimlen);
  }
  return 0;
}


int CheckurlManager::dbinquery(CheckurlManager* imanager,const char *url,const char *domain,int *count) {
  char **dbResult;
  int nRow,nColumn;
  char tmp[1024]={0};
  char tmp_trim[1024]={0};
  char tmp_lst[1024]={0};
  int rc;
  char *sql = tmp;
  char *trim_mid = tmp_trim;
  char *trim_last = tmp_lst;
  int urlint = urlIfcomplete(url);//get protocol_header
  int urllen = strlen(url);//url total length
  char *zErrMsg = 0;

  if(urlint == 1) {
    strncpy(trim_mid,url+7,urllen-7);
    strprocess(trim_mid,&trim_last);
    LOG(INFO) << "urlint==1:trim_last:"<<trim_last;
  } else if(urlint ==2) {
    strncpy(trim_mid,url+8,urllen-8);
    strprocess(trim_mid,&trim_last);
    LOG(INFO) << "urlint==2:trim_last:"<<trim_last;
  } else {
    strcpy(trim_last,url);
    LOG(INFO) << "urlint==other:trim_last:"<<trim_last;
  }

  sprintf(sql,"SELECT * FROM TOPSITES WHERE (SCAN_URL LIKE \"%c%s\");",'%',domain);//topsites模糊查询
  printf("1:sql:%s\n",sql);
	rc = sqlite3_get_table(imanager->checkdb,sql,&dbResult,&nRow,&nColumn,&zErrMsg);

	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		*count = nRow;
    *count = -1;
	}

  if ((nRow == 0) || (rc != SQLITE_OK)) {
	  sqlite3_free_table(dbResult);
    memset(sql,0,1024*sizeof(char));
    sprintf(sql,"SELECT * FROM VIRUS WHERE (SCAN_URL LIKE \"%c%s\");",'%',domain);//virus模糊查询
    printf("2:sql:%s\n",sql);
    rc = sqlite3_get_table(imanager->checkdb,sql,&dbResult,&nRow,&nColumn,&zErrMsg);

    if(rc != SQLITE_OK) {
      fprintf(stderr,"SQL error: %s\n",zErrMsg);
      sqlite3_free(zErrMsg);
    } else {
      *count = nRow;
    }
  }

  sqlite3_free_table(dbResult);

  return 0;
}


namespace extensions {
  namespace checkmurl = api::checkmurl;
  CheckmurlCheckurlFunction::CheckmurlCheckurlFunction(){}

  bool CheckmurlCheckurlFunction::RunAsync() {
    int cnt = 0;
    std::unique_ptr<::extensions::checkmurl::Checkurl::Params> params(
    ::extensions::checkmurl::Checkurl::Params::Create(*args_));

    CheckurlManager* im = g_browser_process->check_url_manager();
    urls = (char*) params->turl.c_str();

    GURL url = GURL(params->turl);
    const char *urldomain = net::registry_controlled_domains::GetDomainAndRegistry(url.host(),net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES).c_str();
    const char *urlmap = strdup(urldomain);
    //printf("before_insert_map_url:%s\n",urldomain);
    std::map <std::string,int>::iterator it;
    it = im->enumMap.find(urldomain);
    if(it != im->enumMap.end()) {//found in map
      //printf("str:%s,value:%d\n",it->first.c_str(),it->second);
      cnt = it->second;
      results_=checkmurl::Checkurl::Results::Create(cnt);
      SendResponse(true);
    }
    else {//not found
      //printf("Not Found\n");
      //2nd:inquery from DB
      im->dbinquery(im,(const char*)params->turl.c_str(),urldomain,&cnt);
      if(cnt > 0) {//1) exist in virus table : block
        results_=checkmurl::Checkurl::Results::Create(cnt);
        SendResponse(true);
      }
      else if(cnt == -1) {//2) exist in topsites table: pass
        cnt = 0;//set cnt to 0 and copy to results_
        results_=checkmurl::Checkurl::Results::Create(cnt);
        SendResponse(true);
      }
      else {
        //3rd:inquery from virustotal
        const Responsecallback callback = base::Bind(&CheckmurlCheckurlFunction::SendResult, this);
        im->CheckUrlTask((char* )params->turl.c_str(), callback,im,urlmap);
        return true;
      }
      //insert data into map
      //printf("insert_map_url:%s\n",urlmap);
      im->enumMap.insert(std::make_pair(urlmap,cnt));
      //int nSize1 = im->enumMap.size();
      //printf("map size:%d\n",nSize1);
    }

    return true;
  }

  void CheckmurlCheckurlFunction::SendResult(int result) {
    LOG(INFO) << "Send Response";
    if(result == 1) {//result from virustotal
      CheckurlManager* im = g_browser_process->check_url_manager();
      im->dbinsert(im,urls);
    }

    results_=checkmurl::Checkurl::Results::Create(result);
    SendResponse(true);
  }
}
