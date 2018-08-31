#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#include "third_party/jansson/src/jansson.h"
#include "VtUrl.h"
#include "VtResponse.h"
#include "cJSON.h"


#define DBG(FMT,ARG...) fprintf(stderr, "%s:%d: " FMT, __FUNCTION__, __LINE__, ##ARG);


void print_usage(const char *prog_name) {
  printf("%s < --apikey YOUR_API_KEY >  [ --all-info ] [ --report-scan ]  [ --report URL ] [ --scan URL ]\n", prog_name);
  printf("  --apikey YOUR_API_KEY   Your virus total API key.  This arg 1st \n");
  printf("  --all-info              When doing a report, set allinfo flag\n");
  printf("  --report-scan           When doing a report, set scan flag\n");
  printf("  --scan URL              URL to scan. \n");
  printf("  --report URL            URL to report.\n");

}


cJSON * parseJson(char * pMsg) {
	if(NULL == pMsg) {
		printf("pMsg is NULL\n");
		exit(1);
	}
	cJSON * pJson = cJSON_Parse(pMsg);
	if(NULL == pJson) {
		printf("pJson is NULL\n");
		exit(1);
	}

	// get string from json
	cJSON * pSub = cJSON_GetObjectItem(pJson, "scans");
	if(NULL == pSub) {
		printf("There is no scans contents in json\n");
	//	exit(1);
	}
	return pSub;
}


char *parseJsonRes(cJSON *psub,int cnt) {
  char *res = NULL;
	if(psub == NULL) {   
res = "No Virus";
			cJSON_Delete(psub);
	}
  else {
		while (psub->child ) {
		  int count = 0;
			cJSON *pSubchildres = psub->child->child;
			
			while (pSubchildres && strcasecmp(pSubchildres->string,"result"))
				pSubchildres = pSubchildres->next;

      //not equal to "clean site" or "unrated iste"
			if(strcasecmp(pSubchildres->valuestring,"clean site") && strcasecmp(pSubchildres->valuestring,"unrated site")) {	
			  if(!strcasecmp(psub->child->string,"Kaspersky") || !strcasecmp(psub->child->string,"Rising") || !strcasecmp(psub->child->string,"Tencent")) {
					cnt = cnt +1;
					res = strdup(pSubchildres->valuestring);
				}
				else if(!strcasecmp(psub->child->string,"Google Safebrowsing") || !strcasecmp(psub->child->string,"G-Data")) {
				  if(count == 2) {
						cnt = cnt +1;
						res = strdup(pSubchildres->valuestring);
					}
					count = count + 1;
				}
			}
			psub->child = psub->child->next;

			if(cnt == 0) {
				res = "No Virus";
				cJSON_Delete(pSubchildres);
			}
			else
				cJSON_Delete(pSubchildres);
		}
	}
	return res;
}


char * jsonFinParse(char *stringjson) {
	int count = 0;
	cJSON *sJson = parseJson(stringjson);
	char *resultjson = parseJsonRes(sJson,count);
	return resultjson;
}


int checkapi(char *apikey,char *url) {
	struct VtUrl *file_scan;
	struct VtResponse *response;
	int ret = 0;
	bool all_info = false;
 	bool report_scan = false;
	char *str = NULL;
	file_scan = VtUrl_new();
	VtUrl_setApiKey(file_scan, apikey);
	if (!apikey) {
		printf("Must set --apikey first\n");
		exit(1);
	}
	ret = VtUrl_report(file_scan, url, all_info, report_scan);
	//DBG("rescan ret=%d\n", ret);
	if (ret) {
		if(ret > 0)
			printf("curl_easy_perform() failed..Check network connection please!! ret is %d\n",ret);
		else if(ret < 0)
		 	printf("virustotal post issue : one public api key limitation: 4 quests per minute!! ret is %d \n", ret);
	} 
	else {
		response = VtUrl_getResponse(file_scan);
		str = VtResponse_toJSONstr(response, VT_JSON_FLAG_INDENT);
		if (str) {
		  char *rs = jsonFinParse(str);//scan results
		  printf("%s\n",rs);
		  free(str);
		}
		VtResponse_put(&response);
	}

	//DBG("Cleanup\n");
	VtUrl_put(&file_scan);
	return 0;
}


int geturlrst(char* rul) {
  char *apikeystr="4e30c69abbb5efed4df5f479e2e7462682d99cd562dfa77897bd497fd4f3bcbc";
  if(rul == NULL) 
		return 0;
  checkapi(apikeystr,rul);
  return 0;
}

