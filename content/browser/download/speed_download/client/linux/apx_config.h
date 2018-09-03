#ifndef APX_CONFIG_H_20150210
#define APX_CONFIG_H_20150210

#include "content/browser/download/speed_download/include/apx_list.h"
#include "content/browser/download/speed_download/include/apx_hftsc_api.h"

#include "content/browser/download/speed_download/client/linux/apx_task.h"
#include "content/browser/download/speed_download/client/linux/apx_user.h"

#include "third_party/uci/uci.h"

#define DEFAULT_ACT_TASK	5
#define DEFAULT_NEXT_TASKID	1

// define the download use data 
extern char DEFAULT_CONFIG[128];
extern char TASK_INFO[128];

struct apx_task_st;

struct apx_config_st
{
	u16 active_task_limit;
	u64 next_taskid;
	char config_file[128];
	char log_file[128];
	struct apx_userinfo_st *userinfo;
	u8 conf_init_flag;
};

int parse_conf_file (char conf_file[],const unsigned int conf_len,char name[],const unsigned int name_len,char path[],const unsigned int path_len);

#endif
