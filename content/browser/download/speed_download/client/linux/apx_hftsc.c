#include "content/browser/download/speed_download/client/linux/apx_hftsc.h"
#include "content/browser/download/speed_download/client/linux/apx_wlog.h"

void apx_hftsc_init(char *fpath_conf)
{
        int ret = 0;
        char logfile[128] = {0};

	apx_conf_init(fpath_conf);

        ret = apx_conf_log_get(logfile, sizeof(logfile));
        if(ret != 0)
        	return;

        ret = apx_wlog_init(logfile);
        if(ret != 0)
                return;
        
	ret = apx_task_init();
        if (ret != 0)
                apx_wlog_quit("transfer module init failed", ret);
}

void apx_hftsc_exit()
{
	int ret;
	apx_task_exit();
	ret = apx_conf_release();
	apx_wlog_release();
}


