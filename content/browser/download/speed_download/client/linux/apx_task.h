#ifndef APX_TASK_H_20150210
#define APX_TASK_H_20150210

#include "content/browser/download/speed_download/include/apx_list.h"
#include "content/browser/download/speed_download/include/apx_hftsc_api.h"

#include "content/browser/download/speed_download/client/linux/apx_file.h"
#include "content/browser/download/speed_download/client/linux/apx_config.h"
#include "content/browser/download/speed_download/client/linux/apx_user.h"
#include "content/browser/download/speed_download/client/linux/apx_transfer_cl.h"
#include "content/browser/download/speed_download/client/linux/apx_transfer_a2.h"

#define TASK_MAX_CONCUR	100

struct apx_task_st
{
	struct list_head list;
	struct apx_userinfo_st *uinfo;

	/* transfer info */
	int trans_handle;
	struct apx_trans_opt trans_opt;
	struct apx_trans_stat trans_stat;
	
	u64 taskid;
	
	time_t	create_time;
	time_t	start_time;	
	time_t	last_start_time;
	time_t	end_time;
	time_t	during_time;
	
	struct timeval	tick_time;	
	u32		tick_down_bytes, tick_up_bytes;
	
	struct trans_operations *trans_opration;
	
	pthread_mutex_t	nref_lock;
	u32		nref;
};

#endif
