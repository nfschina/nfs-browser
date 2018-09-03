#include <sys/types.h>
#include <sys/stat.h>

#include "content/browser/download/speed_download/client/linux/apx_task.h"
#include "content/browser/download/speed_download/client/linux/apx_wlog.h"

struct trans_operations {
	int (*init) (u32 index);
	int (*uri_check)(struct apx_trans_opt* task_opt);
	int (*create) (void);
	int (*recv) (u32 nu);
        int (*get_opt) (u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt);
	int (*get_stat) (u32 nu, struct apx_trans_stat* task_stat);
	int (*start) (u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt);
	int (*stop) (u32 nu);
	int (*destroy) (u32 nu);
	int (*del_file) (struct apx_trans_glboptions *glb_opt, struct apx_trans_opt *task_opt);
	int (*get_btfile) (u32 nu, struct apx_trans_opt* task_opt, struct btfile* bt_file);
	int (*release) (u32 nu, int flags);
	void (*exit) (void);
};

struct trans_operations g_trans_a2_op =
{
    .init       = apx_trans_init_a2,
    .uri_check  = apx_trans_precreate_a2,
    .start      = apx_trans_start_a2,
    .create     = apx_trans_create_a2,
    .get_stat   = apx_trans_getstat_a2,
    .destroy    = apx_trans_delete_a2,
    .del_file   = apx_trans_del_file_a2,
    .get_btfile = apx_trans_get_btfile_a2,
    .release    = apx_trans_release_a2,
    .exit       = apx_trans_exit_a2
};

struct trans_operations g_trans_cl_op =
{
    .init       = apx_trans_init_cl,
    .uri_check	= apx_trans_precreate_cl,
    .create     = apx_trans_create_cl,
    .recv	= apx_trans_recv_cl,
    .get_opt	= apx_trans_getopt_cl,
    .get_stat	= apx_trans_getstat_cl,
    .start	= apx_trans_start_cl,
    .stop	= apx_trans_stop_cl,
    .destroy	= apx_trans_delete_cl,
    .del_file	= apx_trans_del_file_cl,
//    .get_btfile	= apx_trans_get_btfile_cl,
    .release	= apx_trans_release_cl,
    .exit	= apx_trans_exit_cl
};

struct trans_operations *g_trans_op[APX_TASK_PROTO_END];

static int __apx_user_task_traverse_all(u32 uid, int func(struct apx_task_st *task))
{
	if (func == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	struct apx_task_st *p = NULL, *tmp = NULL;
	int mode;

	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;

	for (mode = APX_TASK_STATE_START; mode < APX_TASK_STATE_END; mode++)
	{
		list_for_each_entry_safe( p, tmp, &userinfo->user_task.task_list[mode], list)
		func((void *)(p));
	}

	return 0;
}

static struct apx_task_st *taskinfo_alloc()
{
	struct apx_task_st *task = malloc(sizeof(struct apx_task_st));

	if (task == NULL)
		return NULL;

	memset(task, 0, sizeof(struct apx_task_st));
	INIT_LIST_HEAD(&task->list);
	pthread_mutex_init(&task->nref_lock, NULL);
	task->trans_handle = -1;

	return task;
}

static void taskinfo_free(struct apx_task_st *task)
{
	pthread_mutex_destroy(&task->nref_lock);
	free(task);
}

static void tinfo_hold(struct apx_task_st *task)
{
	pthread_mutex_lock(&task->nref_lock);
	task->nref++;
	pthread_mutex_unlock(&task->nref_lock);
}

static void tinfo_put(struct apx_task_st *task)
{
	pthread_mutex_lock(&task->nref_lock);
	task->nref--;
	pthread_mutex_unlock(&task->nref_lock);
}

static int __utask_list_del(struct apx_userinfo_st	*uinfo, struct apx_task_st *task)
{
	if (uinfo == NULL || task == NULL)
		return -1;

	list_del_init(&task->list);
	uinfo->user_task.task_num[task->trans_stat.state]--;
	task->trans_stat.state = APX_TASK_STATE_UNDEFINED;
	tinfo_put(task);

	return 0;
}

static int utask_list_del(struct apx_userinfo_st	*uinfo, struct apx_task_st *task)
{
	int ret;

	pthread_mutex_lock(&uinfo->user_task.task_list_lock);
	ret = __utask_list_del(uinfo, task);
	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);

	return ret;
}

static int __utask_list_add(struct apx_userinfo_st *uinfo, struct apx_task_st *task, u8 state)
{
	struct apx_task_st *tmp = NULL;

	if (uinfo == NULL || task == NULL || state < APX_TASK_STATE_START || state >= APX_TASK_STATE_END)
		return -1;

	list_for_each_entry(tmp, &uinfo->user_task.task_list[state], list)
	{
		if (task->trans_opt.priv < tmp->trans_opt.priv)
			break;
	}

	list_add_tail(&task->list, &tmp->list);
	uinfo->user_task.task_num[state]++;
	tinfo_hold(task);

	task->trans_stat.state = state;

	return 0;
}

static int utask_list_add(struct apx_userinfo_st	*uinfo, struct apx_task_st *task, u8 state)
{
	int ret;

	pthread_mutex_lock(&uinfo->user_task.task_list_lock);
	ret = __utask_list_add(uinfo, task, state);
	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);

	return ret;
}

static int __utask_list_move(struct apx_userinfo_st	*uinfo, struct apx_task_st *task, u8 state)
{
	int ret;

	ret = __utask_list_del(uinfo, task);
	ret = __utask_list_add(uinfo, task, state);

	return ret;
}

static int utask_list_move(struct apx_userinfo_st	*uinfo, struct apx_task_st *task, u8 state)
{
	int ret;

	pthread_mutex_lock(&uinfo->user_task.task_list_lock);
	ret =  __utask_list_move(uinfo, task, state);
	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);

	return ret;
}

static struct apx_task_st *taskid2taskinfo(u64 taskid)
{
	int i;
	struct apx_task_st * task = NULL;
	struct apx_userinfo_st *uinfo = NULL;

	uinfo = apx_conf_uinfo_get();
	if (uinfo == NULL)
		return NULL;

	pthread_mutex_lock(&uinfo->user_task.task_list_lock);
	/* traverse three task list of the user to find the  corresponding task by taskid */
	for (i = APX_TASK_STATE_START; i < APX_TASK_STATE_END; i++)
	{
		list_for_each_entry(task, &uinfo->user_task.task_list[i], list)
		{
			if (task->taskid == taskid)
				goto find_out;
		}
	}

	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);
	return NULL;

find_out:
	/* If find ,we increase the count of refers */
	tinfo_hold(task);
	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);
	return task;
}

static u8 task_proto_from_uri(char* uri)
{
	char buff[32] = {0};

	if (uri == NULL)
		return APX_TASK_PROTO_UNKNOWN;

	strncpy(buff, uri, sizeof(buff) - 1);

	if (!strncasecmp(uri, "https", strlen("https")))
		return APX_TASK_PROTO_HTTPS;
	else if (!strncasecmp(uri, "http", strlen("http")))
		return APX_TASK_PROTO_HTTP;
	else if (!strncasecmp(uri, "ftp", strlen("ftp")))
		return APX_TASK_PROTO_FTP;
        else if (!strncasecmp(uri, "/", strlen("/")) || !strncasecmp(uri, "./", strlen("./")))
                return APX_TASK_PROTO_BT;
//          else if (!strncasecmp(uri, "data", strlen("data")))
//                     return APX_TASK_PROTO_DATA;
	else
		return APX_TASK_PROTO_UNKNOWN;
}

static int __task2active(struct apx_task_st *task)
{
	int ret = 0;

	/* check task current state */
	if (task == NULL || task->trans_stat.state != APX_TASK_STATE_WAIT)
		return -1;

    if (task->trans_handle == -1)
    {
    	task->trans_handle = task->trans_opration->create();
        if (task->trans_handle == -1)
        	return -2;
    }

	ret = task->trans_opration->start(task->trans_handle,NULL,&task->trans_opt);
	if (ret != 0)
	{
		// if the task is already downloaded, then move to finished
		if (ret == -7)
		{
			/* move task node to finished tasklist */
            __utask_list_del(task->uinfo, task);
            __utask_list_add(task->uinfo, task, APX_TASK_STATE_FINISHED);
			return 0;
		}
        else
        {
            __utask_list_del(task->uinfo, task);
            __utask_list_add(task->uinfo, task, APX_TASK_STATE_TERMINATED);
            return 0;
        }

		return -3;
	}

	/* move task node to active tasklist */
	__utask_list_del(task->uinfo, task);
	__utask_list_add(task->uinfo, task, APX_TASK_STATE_ACTIVE);

        tinfo_save(task);

	task->last_start_time	=	task->start_time;
	task->start_time		=	time(NULL);
	return 0;
}

static int __task_trans_release(struct apx_task_st *task, int del)
{
	int ret = 0, flags = 1;
	if (task->trans_handle == -1)
	{
		/* the trans handle had been released before */
		if (del)
			ret = task->trans_opration->del_file(NULL, &task->trans_opt);
		return ret;
	}

	ret = task->trans_opration->get_stat(task->trans_handle, &task->trans_stat);
        if (ret)
                apx_wlog("apx_trans_get_stat failed", ret);

	if (del)
	    ret = task->trans_opration->destroy(task->trans_handle);
	else
	    ret = task->trans_opration->release(task->trans_handle, flags);

        if (ret)
                apx_wlog("apx_trans_release/destroy failed", ret);

	task->trans_handle			= -1;
	task->end_time				= time(NULL);
	task->during_time			+= task->end_time - task->start_time;
	task->trans_stat.down_speed = 0;
	task->trans_stat.up_speed 	= 0;
	return ret;
}

static int __task2finish(struct apx_task_st *task)
{
	/* check task current state */
	if (task == NULL || task->trans_stat.state != APX_TASK_STATE_ACTIVE)
		return -1;

	__task_trans_release(task, 0);

	/* move task node to finished tasklist */
	__utask_list_del(task->uinfo, task);
	__utask_list_add(task->uinfo, task, APX_TASK_STATE_FINISHED);

	return 0;
}

static int __task2wait(struct apx_task_st *task)
{
	//int ret = 0, flags = 1;

	/* check task current state */
	if (task == NULL || (task->trans_stat.state != APX_TASK_STATE_ACTIVE
		&& task->trans_stat.state != APX_TASK_STATE_STOP
		&& task->trans_stat.state != APX_TASK_STATE_TERMINATED))
		return -1;

	/* active -> wait*/
	if (task->trans_stat.state == APX_TASK_STATE_ACTIVE)
		__task_trans_release(task, 0);

	/* move task node to wait tasklist */
	__utask_list_del(task->uinfo, task);
	__utask_list_add(task->uinfo, task, APX_TASK_STATE_WAIT);

	return 0;
}

static int task2wait(struct apx_task_st *task)
{
	int ret = 0;

	pthread_mutex_lock(&task->uinfo->user_task.task_list_lock);
	ret = __task2wait(task);
	pthread_mutex_unlock(&task->uinfo->user_task.task_list_lock);

	return ret;
}

static int __task2stop(struct apx_task_st *task)
{
	//int ret = 0, flags = 1;

	/* check task current state */
	if (task == NULL || task->trans_stat.state < APX_TASK_STATE_ACTIVE
		|| task->trans_stat.state >= APX_TASK_STATE_FINISHED)
		return -1;

	/* already stopped */
	if (task->trans_stat.state == APX_TASK_STATE_STOP)
		return 0;

	/* active -> stop */
	if (task->trans_stat.state == APX_TASK_STATE_ACTIVE)
		__task_trans_release(task, 0);

	/* move task node to stop tasklist */
	__utask_list_move(task->uinfo, task, APX_TASK_STATE_STOP);

	return 0;
}

static int __task2terminated(struct apx_task_st *task)
{
	//int ret = 0, flags = 1;

	/* check task current state */
	if (task == NULL || task->trans_stat.state < APX_TASK_STATE_ACTIVE
		|| task->trans_stat.state >= APX_TASK_STATE_FINISHED)
		return -1;

	/* active -> stop */
	if (task->trans_stat.state == APX_TASK_STATE_ACTIVE)
		__task_trans_release(task, 0);

	/* move task node to terminated tasklist */
	__utask_list_move(task->uinfo, task, APX_TASK_STATE_TERMINATED);

	return 0;
}

static int task2stop(struct apx_task_st *task)
{
	int ret = 0;

	pthread_mutex_lock(&task->uinfo->user_task.task_list_lock);
	ret = __task2stop(task);
	pthread_mutex_unlock(&task->uinfo->user_task.task_list_lock);

	return ret;
}

static int task2bedel(struct apx_task_st *task)
{
	int ret = 0;

	pthread_mutex_lock(&task->uinfo->user_task.task_list_lock);

	if (task->trans_stat.state == APX_TASK_STATE_FINISHED)
		__utask_list_move(task->uinfo, task, APX_TASK_STATE_FINTOBEDEL);
	else if (task->trans_stat.state == APX_TASK_STATE_TERMINATED)
	{
		__utask_list_move(task->uinfo, task, APX_TASK_STATE_TOBEDEL);
	}
	else
	{
		ret = __task2stop(task);
	}

	pthread_mutex_unlock(&task->uinfo->user_task.task_list_lock);

	return ret;
}

int tinfo_save(struct apx_task_st *task)
{
	struct uci_package *pkg = NULL;
	struct uci_context *ctx = NULL;
	struct uci_ptr ptr;
	char file_name[128] = {0};
	char file_path[128] = {0};
	char buf[128] = {0};
	char id[128] = {0};
	char p[128] = {0};
	int ret = 0;
	//int fd = -1;
	FILE* fp = NULL;

	if (task == NULL)
		return -1;

	int res = parse_conf_file(TASK_INFO, sizeof(TASK_INFO), file_name,
				sizeof(file_name), file_path, sizeof(file_path));
	if (res)
	{
		if (res == -1)
		{
		       apx_wlog("parse_conf_file failed", res);
			return -2;
		}
		umask(0);
		if (res == -2)
		{
			if (mkdir(file_path, 0777))
			{
				apx_wlog("parse_conf_file failed", res);
				return -3;
			}
		}

        //	if ((fd = creat(TASK_INFO, 0666)) == -1)
		if ((fp = fopen(TASK_INFO, "a+")) == NULL)
		{
			apx_wlog("parse_conf_file failed", res);
                        return -4;
		}
        else
            //close(fd);
		    fclose(fp);
	}

	ctx = uci_alloc_context(file_path);
	if (ctx == NULL)
		return -5;

	if (uci_set_confdir(ctx, file_path) != UCI_OK)
	{
		uci_free_context(ctx);
		return -6;
	}

	if (uci_load(ctx, file_name, &pkg) != UCI_OK)
	{
		uci_free_context(ctx);
		return -7;
	}

	snprintf(id, sizeof(id) - 1, "%lld", task->taskid);

	/* if cannot find taskid add section serv */
	if (uci_lookup_section(ctx, pkg, id) == NULL)
	{
		snprintf(buf, sizeof(buf) - 1, "%s.%s", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);
		ptr.value = "task";

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -8;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -9;
			goto out;
		}
	}

	/* save task type */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskType", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%hhu", task->trans_opt.type);
	ptr.value = p;
	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -12;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -13;
		goto out;
	}

	/* save task user id */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.UserId", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%d", task->uinfo->userid);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -14;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -15;
		goto out;
	}

	/* save task priority level */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskLevel", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%hhu", task->trans_opt.priv);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -16;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -17;
		goto out;
	}

	/* save task create time */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.CreatTime", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%ld", task->create_time);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -18;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -19;
		goto out;
	}

	/* save task start time */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.StartTime", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%ld", task->start_time);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -20;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -21;
		goto out;
	}

	/* save task last start time */
	snprintf(buf, sizeof(buf) - 1,"%s.%s.LastStartTime", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%ld", task->last_start_time);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -22;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -23;
		goto out;
	}

	/* save task end time */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.EndTime", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%ld", task->end_time);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -24;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -25;
		goto out;
	}

	/* save task during time */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.DuringTime", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%ld", task->during_time);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -26;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -27;
		goto out;
	}

	/* save task down url */
//    if (task->trans_opt.proto != APX_TASK_PROTO_DATA)
//    {
        snprintf(buf, sizeof(buf) - 1, "%s.%s.DownloadUrl", file_name, id);
        uci_lookup_ptr(ctx, &ptr, buf, true);

        ptr.value = task->trans_opt.uri;
        if (uci_set(ctx, &ptr) == UCI_OK)
        {
            if (uci_save(ctx, ptr.p) != UCI_OK)
            {
                uci_perror(ctx, NULL);
                ret = -28;
                goto out;
            }
        }
        else
        {
            uci_perror(ctx, NULL);
            ret = -29;
            goto out;
        }
//    }

	/* save task bp continue */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskBpContinue", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%hhu", task->trans_opt.bp_continue);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -30;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -31;
		goto out;
	}

	/* save task protocol */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskProto", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%hhu", task->trans_opt.proto);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -32;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -33;
		goto out;
	}

	/* save task ftp user */
	snprintf(buf, sizeof(buf) - 1,"%s.%s.FtpUser", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	ptr.value = task->trans_opt.ftp_user;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -34;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -35;
		goto out;
	}

	/* save task ftp passward */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.FtpPwd", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	ptr.value = task->trans_opt.ftp_passwd;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -36;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -37;
		goto out;
	}

	/* save task bt select */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.BTSelect", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	ptr.value = task->trans_opt.bt_select;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -38;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -39;
		goto out;
	}

	/* save task down path */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.DownloadPath", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	ptr.value = task->trans_opt.fpath;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -40;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -41;
		goto out;
	}

	/* save task down filename */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.DownloadFile", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	ptr.value = task->trans_opt.fname;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -42;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -43;
		goto out;
	}

	/* save task concurrent */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskConcurrent", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%hhu", task->trans_opt.concurr);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -44;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -45;
		goto out;
	}

	/* save task down splimit */
	snprintf(buf, sizeof(buf) - 1, "%s.%s.DownSplimit", file_name, id);
	uci_lookup_ptr(ctx, &ptr, buf, true);

	snprintf(p, sizeof(p) - 1, "%d", task->trans_opt.down_splimit);
	ptr.value = p;

	if (uci_set(ctx, &ptr) == UCI_OK)
	{
		if (uci_save(ctx, ptr.p) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -46;
			goto out;
		}
	}
	else
	{
		uci_perror(ctx, NULL);
		ret = -47;
		goto out;
	}

		/* save task total size */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskTotalSize", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1, "%lld", task->trans_stat.total_size);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -50;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -51;
			goto out;
		}

		/* save task down size */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskDownSize", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1,  "%lld", task->trans_stat.down_size);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -52;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -53;
			goto out;
		}

		/* save task down speed */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskDownSpeed", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1, "%d", task->trans_stat.down_speed);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -56;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -57;
			goto out;
		}

		/* save task connections */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskConNum", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1, "%hd", task->trans_stat.connections);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -60;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -61;
			goto out;
		}

		/* save task state */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskState", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1, "%hhu", task->trans_stat.state);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -62;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -63;
			goto out;
		}

		/* save task state */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskStateEvent", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1, "%hhu", task->trans_stat.state_event);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -64;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -65;
			goto out;
		}
		/* save task errno */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskTransErrno", file_name, id);
		uci_lookup_ptr(ctx, &ptr, buf, true);

		snprintf(p, sizeof(p) - 1, "%d", task->trans_stat.trans_errno);
		ptr.value = p;

		if (uci_set(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, ptr.p) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -68;
				goto out;
			}
		}
		else
		{
			uci_perror(ctx, NULL);
			ret = -69;
			goto out;
		}
		if (uci_commit(ctx, &pkg, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -70;
		}
	out:
		uci_unload(ctx, pkg);
		uci_free_context(ctx);
		return  ret;
	}

	int tinfo_load(struct apx_userinfo_st *uinfo)
	{
		struct uci_package *pkg = NULL;
		struct uci_context *ctx = NULL;
		struct uci_element *e = NULL;
		struct uci_ptr ptr;
		char file_name[128] = {0};
		char file_path[128] = {0};
		char buf[128] = {0};
		int ret = 0;
		int res = 0;

		if (uinfo == NULL)
			return -1;

		res = parse_conf_file(TASK_INFO, sizeof(TASK_INFO), file_name,
				sizeof(file_name), file_path, sizeof(file_path));
		if (res)
		{
			apx_wlog("tinfo_load parse conf file failed", res);
			return -2;
		}
		ctx = uci_alloc_context(file_path);
		if (ctx == NULL)
			return -3;

		if (uci_set_confdir(ctx, file_path) != UCI_OK)
		{
			uci_free_context(ctx);
			return -4;
		}
		if (uci_load(ctx, file_name, &pkg) != UCI_OK)
		{
			uci_free_context(ctx);
			return -5;
		}

		uci_foreach_element(&pkg->sections, e)
		{
			struct uci_section *s = uci_to_section(e);
			if (strncmp(s->type, "task", strlen("task")) != 0)
				continue;

			struct apx_task_st *taskinfo = NULL;
			 taskinfo = taskinfo_alloc();
			if (taskinfo == NULL)
			{
				ret = -6;
				goto out1;
			}

			taskinfo->taskid = atoll(s->e.name);

			/* get task user id */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.UserId", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -7;
				goto out;
			}
			else
			{
				if ( (uinfo->userid != (unsigned int)atol(ptr.o->v.string)) && (uinfo->userid != 0))
				{
					taskinfo_free(taskinfo);
					continue;
				}
			}

			/* get task server task id */
/*			snprintf(buf, sizeof(buf) - 1, "%s.%s.ServTaskId", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -8;
				goto out;
			}
			else
				taskinfo->serv_taskid = atoll(ptr.o->v.string);*/

			/* get task type */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskType", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -9;
				apx_wlog("get task task type failed", ret);
				goto out;
			}
			else
				taskinfo->trans_opt.type = atoi(ptr.o->v.string);

			/* get task  priority level */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskLevel", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -10;
				apx_wlog("get task task priority level failed", ret);
				goto out;
			}
			else
				taskinfo->trans_opt.priv = atoi(ptr.o->v.string);

			/* get task create time */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.CreatTime", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -11;
				apx_wlog("get task creat time failed", ret);
				goto out;
			}
			else
				taskinfo->create_time = atol(ptr.o->v.string);

			/* get task start time */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.StartTime", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -12;
				apx_wlog("get task start time failed", ret);
				goto out;
			}
			else
				taskinfo->start_time = atol(ptr.o->v.string);

			/* get task last start time */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.LastStartTime", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -13;
				apx_wlog("get task last start time failed", ret);
				goto out;
			}
			else
				taskinfo->last_start_time = atol(ptr.o->v.string);

			/* get task end time */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.EndTime", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -14;
				apx_wlog("get task end time failed", ret);
				goto out;
			}
			else
				taskinfo->end_time = atol(ptr.o->v.string);

			/* get task during time */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.DuringTime", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -15;
				apx_wlog("get task during time failed", ret);
				goto out;
			}
			else
				taskinfo->during_time = atol(ptr.o->v.string);

			/* get task protocol */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskProto", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -17;
				goto out;
			}
			else
			{
				taskinfo->trans_opt.proto = atoi(ptr.o->v.string);
				if (taskinfo->trans_opt.proto >= APX_TASK_PROTO_END)
				{
					ret = -18;
					goto out;
				}
			}

			/* get task down url */
//            if (taskinfo->trans_opt.proto != APX_TASK_PROTO_DATA)
//            {
                snprintf(buf, sizeof(buf) - 1, "%s.%s.DownloadUrl", file_name, s->e.name);
                if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
                {
                    uci_perror(ctx, NULL);
                    ret = -16;
		    apx_wlog("get task down url failed", ret);
                    goto out;
                }
                else
                {
                    taskinfo->trans_opt.uri =  (char*)malloc(strlen(ptr.o->v.string)+1);
                    strncpy(taskinfo->trans_opt.uri, ptr.o->v.string, strlen(ptr.o->v.string)+1);
                }
//            }

			/* get task ftp user */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.FtpUser", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
				strncpy(taskinfo->trans_opt.ftp_user, "", sizeof(taskinfo->trans_opt.ftp_user) - 1);
			else
				strncpy(taskinfo->trans_opt.ftp_user, ptr.o->v.string, sizeof(taskinfo->trans_opt.ftp_user) - 1);

			/* get task ftp passwd */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.FtpPwd", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
				strncpy(taskinfo->trans_opt.ftp_passwd, "", sizeof(taskinfo->trans_opt.ftp_passwd) - 1);
			else
				strncpy(taskinfo->trans_opt.ftp_passwd, ptr.o->v.string, sizeof(taskinfo->trans_opt.ftp_passwd) - 1);

			/* get task bt select */
			if (taskinfo->trans_opt.proto == APX_TASK_PROTO_BT)
			{
				snprintf(buf, sizeof(buf) - 1, "%s.%s.BTSelect", file_name, s->e.name);
				if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
				{
					uci_perror(ctx, NULL);
					ret = -19;
					apx_wlog("get task BT select failed", ret);
					goto out;
				}
				else
					strncpy(taskinfo->trans_opt.bt_select, ptr.o->v.string, sizeof(taskinfo->trans_opt.bt_select) - 1);
			}

			/* get task down path */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.DownloadPath", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				apx_wlog("get task down file path failed", ret);
				ret = -20;
				goto out;
			}
			else
				strncpy(taskinfo->trans_opt.fpath, ptr.o->v.string, sizeof(taskinfo->trans_opt.fpath) - 1);

			/* get task down filename */
			//if (taskinfo->trans_opt.proto != APX_TASK_PROTO_BT)
			//{
				snprintf(buf, sizeof(buf) - 1, "%s.%s.DownloadFile", file_name, s->e.name);
				if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
				{
					uci_perror(ctx, NULL);
					ret = -21;
					apx_wlog("get task down filename failed", ret);
					goto out;
				}
				else
					strncpy(taskinfo->trans_opt.fname, ptr.o->v.string, sizeof(taskinfo->trans_opt.fname) - 1);
			//}

			/* get task concurrent */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskConcurrent", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				ret = -22;
				apx_wlog("get task concurrent failed", ret);
				goto out;
			}
			else
				taskinfo->trans_opt.concurr = atoi(ptr.o->v.string);

			/* get task down splimit */
			snprintf(buf, sizeof(buf) - 1, "%s.%s.DownSplimit", file_name, s->e.name);
			if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
			{
				uci_perror(ctx, NULL);
				 apx_wlog("get task down speed limit failed", ret);
				ret = -23;
				goto out;
			}
			else
				taskinfo->trans_opt.down_splimit = atoi(ptr.o->v.string);

			/* get task up splimit */
	//		snprintf(buf, sizeof(buf) - 1, "%s.%s.UpSplimit", file_name, s->e.name);
	//		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
	//		{
	//			uci_perror(ctx, NULL);
	//			ret = -24;
	//			apx_wlog("get task up speed limit failed", ret);
	//			goto out;
	//		}
	//		else
	//			taskinfo->trans_opt.up_splimit = atoi(ptr.o->v.string);

		/* get task total size */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskTotalSize", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -25;
			apx_wlog("get task total size failed", ret);
			goto out;
		}
		else
		{
			taskinfo->trans_stat.total_size = atoll(ptr.o->v.string);
			taskinfo->trans_opt.fsize = atoll(ptr.o->v.string);
		}

		/* get task bp continue */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskBpContinue", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -26;
			apx_wlog("get task bp continue failed", ret);
			goto out;
		}
		else
			taskinfo->trans_opt.bp_continue = atoi(ptr.o->v.string);

		/* get task down size */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskDownSize", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -27;
			apx_wlog("get task down size failed", ret);
			goto out;
		}
		else
			taskinfo->trans_stat.down_size = atoll(ptr.o->v.string);

		/* get task up size */
	/*	snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskUpSize", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -28;
			goto out;
		}
		else
			taskinfo->trans_stat.up_size = atoll(ptr.o->v.string);
*/
		/* get task down speed */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskDownSpeed", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -29;
			apx_wlog("get task down speed failed", ret);
			goto out;
		}
		else
			taskinfo->trans_stat.down_speed = atoi(ptr.o->v.string);

		/* get task up speed */
/*		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskUpSpeed", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -30;
			goto out;
		}
		else
			taskinfo->trans_stat.up_speed = atoi(ptr.o->v.string);
*/
		/* get task connections */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskConNum", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -31;
			apx_wlog("get task connections failed", ret);
			goto out;
		}
		else
			taskinfo->trans_stat.connections = atoi(ptr.o->v.string);

		/* get task errno */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskTransErrno", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -32;
			apx_wlog("get task server task errno failed", ret);
			goto out;
		}
		else
			taskinfo->trans_stat.trans_errno = atoi(ptr.o->v.string);

		/* get task file id */
	//    if (taskinfo->trans_opt.type == APX_TASK_TYPE_SERVER_UP || taskinfo->trans_opt.type == APX_TASK_TYPE_SERVER_DOWN)
        //{
	//	    snprintf(buf, sizeof(buf) - 1, "%s.%s.fileId", file_name, s->e.name);
//		    if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
//		    {
//			    uci_perror(ctx, NULL);
//			    ret = -33;
//			    apx_wlog("get task down url failed", ret);
//			    goto out;
//		    }
//		    else
//			    strncpy(taskinfo->trans_opt.fileId, ptr.o->v.string, sizeof(taskinfo->trans_opt.fileId) - 1);
//        }


		/* get task state */
		snprintf(buf, sizeof(buf) - 1, "%s.%s.TaskState", file_name, s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
		{
			uci_perror(ctx, NULL);
			ret = -34;
			apx_wlog("get task state failed", ret);
			goto out;
		}
		else
		{
			taskinfo->trans_stat.state = atoi(ptr.o->v.string);
			if (taskinfo->trans_stat.state == APX_TASK_STATE_ACTIVE)
				taskinfo->trans_stat.state = APX_TASK_STATE_TERMINATED;

			/* get task tick time */
			gettimeofday(&taskinfo->tick_time, NULL);
			taskinfo->uinfo = uinfo;
			taskinfo->trans_opration = g_trans_op[taskinfo->trans_opt.proto];

			/* add task list */
			utask_list_add(uinfo, taskinfo, taskinfo->trans_stat.state);

			continue;
		}
out:
		taskinfo_free(taskinfo);
	}
out1:
	uci_unload(ctx, pkg);
	uci_free_context(ctx);
	return ret;
}

int tinfo_delete(struct apx_task_st *task)
{
	struct uci_package *pkg = NULL;
	struct uci_context *ctx = NULL;
	struct uci_element *e = NULL;
	struct uci_ptr ptr = {0};
	char file_name[128] = {0};
	char file_path[128] = {0};
	char buf[128] = {0};
	int ret = 0;
	int res = 0;

	if (task == NULL)
		return -1;

	res = parse_conf_file(TASK_INFO, sizeof(TASK_INFO), file_name,
			sizeof(file_name), file_path, sizeof(file_path));
	if (res)
	{
		apx_wlog("tinfo_delete parse conf file failed", res);
		return -2;
	}

	ctx = uci_alloc_context(file_path);
	if (ctx == NULL)
		return -3;

	if (uci_set_confdir(ctx, file_path) != UCI_OK)
	{
		uci_free_context(ctx);
		return -4;
	}
	if (uci_load(ctx, file_name, &pkg) != UCI_OK)
	{
		uci_free_context(ctx);
		return -5;
	}

	uci_foreach_element(&pkg->sections, e)
	{
		struct uci_section *s = uci_to_section(e);
		if (strncmp(s->type, "task", strlen("task")) != 0)
			continue;

		if (task->taskid != strtoull(s->e.name, NULL, 0))
			continue;

		snprintf(buf, sizeof(buf) - 1, "%s.%s", file_name, s->e.name);
		uci_parse_ptr(ctx, &ptr, buf);
		if (uci_delete(ctx, &ptr) == UCI_OK)
		{
			if (uci_save(ctx, pkg) != UCI_OK)
			{
				ret = -6;
				apx_wlog("task info save delete failed", ret);
				goto out;
			}
			break;
		}
		else
		{
			ret = -7;
			apx_wlog("task info delete failed", ret);
			goto out;
		}
	}
	if (uci_commit(ctx, &pkg, false) != UCI_OK)
	{
		ret = -8;
		apx_wlog("task info commit failed", ret);
	}
out:
	uci_unload(ctx, pkg);
	uci_free_context(ctx);

	return ret;
}

/* main process for task management */
void thread_utask_scheduler(void * data)
{
	struct apx_userinfo_st *uinfo;
	struct apx_usertask_st *utask;
	struct apx_task_st *task,*tmp, *last_act_task, *first_wait_task;
	int ret;
	//int count = 0;

	uinfo = (struct apx_userinfo_st *)data;
	utask = &uinfo->user_task;

	while (1)
	{
		pthread_mutex_lock(&utask->task_list_lock);

		/* check active task list, change finished task to finished state  */
		list_for_each_entry_safe(task, tmp, &utask->task_list[APX_TASK_STATE_ACTIVE], list)
		{
            //task_flag = 1;
			struct apx_trans_stat *trans_stat;
			ret = task->trans_opration->get_stat(task->trans_handle, &task->trans_stat);
			if (ret == -1)
				continue;

			trans_stat = &(task->trans_stat);
			//apx_vlog( "[%u],[%d]",trans_stat->state_event, trans_stat->disk_flag);
			/* finish event occured */
			if (trans_stat->state_event == APX_TASK_STATE_FINISHED
				/*  A2 have bug when start several tasks. If first not release complete, the leave can't
				      receive finished event. So we complement two condition to finish task  */
				|| (/*trans_stat->total_size && */trans_stat->total_size == trans_stat->down_size)
				|| (/*trans_stat->total_size && */trans_stat->total_size == trans_stat->up_size && trans_stat->disk_flag == 1 ) )
			{
				ret = __task2finish(task);
			}else if (trans_stat->state_event == APX_TASK_STATE_UNDEFINED){
				/* error event occured */
				//ret = __task2stop(task);
				ret = __task2terminated(task);
			}
		}

		list_for_each_entry_safe(task, tmp, &utask->task_list[APX_TASK_STATE_WAIT], list)
		{
			if (uinfo->active_task_limit != 0 &&  // 0 means unlimited
				uinfo->active_task_limit <= utask->task_num[APX_TASK_STATE_ACTIVE])
				break;

			ret = __task2active(task);
		}

		/* adjust task concurr  (active task num > active task limit) */
		while (uinfo->active_task_limit != 0 && uinfo->active_task_limit < utask->task_num[APX_TASK_STATE_ACTIVE])
		{
			last_act_task = list_entry((&utask->task_list[APX_TASK_STATE_ACTIVE])->prev, struct apx_task_st, list);
			__task2wait(last_act_task);
		}

		/* adjust task according privilege (active task num = active task limit) */
		while(1)
		{
			if (utask->task_num[APX_TASK_STATE_ACTIVE] == 0
				|| utask->task_num[APX_TASK_STATE_WAIT] == 0)
				break;

			first_wait_task = list_first_entry(&utask->task_list[APX_TASK_STATE_WAIT], struct apx_task_st, list);
			last_act_task = list_entry((&utask->task_list[APX_TASK_STATE_ACTIVE])->prev, struct apx_task_st, list);

			if (last_act_task->trans_opt.priv >= first_wait_task->trans_opt.priv)
				break;

			__task2stop(last_act_task);
			__task2active(first_wait_task);
		}

		/* every 5 minutes, the thread output all task info to tasklog file */
		//if (count % 300 == 5)
		//{
		//	apx_conf_writeback();
		//	__apx_user_task_traverse_all(current_uid_get(), tinfo_save);
		//}

		pthread_mutex_unlock(&utask->task_list_lock);
		sleep(1);
		//count++; 		//use to debug
	}
}

int apx_task_restore(int uid)
{
	int ret = 0;
	struct apx_userinfo_st	*uinfo = NULL;

	uinfo = uid2uinfo(current_uid_get());
	if (uinfo == NULL)
		return -1;

	ret = tinfo_load(uinfo);
	if (ret != 0)
                apx_wlog("load task failed", ret);

	return ret;
}

static int __apx_task_release_save(struct apx_task_st *task)
{
	int ret;
	if (task == NULL)
		return -1;

	__task_trans_release(task, 0);

	/*  save task info to disk  */
	ret = tinfo_save(task);
        if (ret != 0)
                apx_wlog("save task info failed", ret);

	/* delete task node from usertask list first,so the other thread couldn't find it again */
	__utask_list_del(task->uinfo, task);
	/* wait other thread release task node */
	while (task->nref != 0)
		usleep(100000);

	taskinfo_free(task);

	return 0;
}

static int apx_user_task_traverse_all(u32 uid, int (func)(struct apx_task_st *task))
{
        if (func == NULL)
                return -2;

        struct apx_userinfo_st *userinfo = NULL;
        int ret = 0;


        userinfo = uid2uinfo(uid);
        if (userinfo == NULL)
                return -1;

		pthread_mutex_lock(&userinfo->user_task.task_list_lock);
        ret = __apx_user_task_traverse_all(uid, func);
		pthread_mutex_unlock(&userinfo->user_task.task_list_lock);

        return ret;
}


static int __apx_task_release(struct apx_task_st *task)
{
	int ret;

	if (task == NULL)
		return -1;

	__task_trans_release(task, 0);

	/* delete task node from usertask list first,so the other thread couldn't find it again */
	__utask_list_del(task->uinfo, task);

	/* wait other thread release task node */
	while (task->nref != 0)
		usleep(100000);

	/*  delete task info from disk  */
	ret = tinfo_delete(task);
        if (ret != 0)
                apx_wlog("delete task from disk failed", ret);

	taskinfo_free(task);

	return 0;
}

int apx_task_release(int taskid)
{
	int ret;
	struct apx_task_st *task = NULL;
	struct apx_userinfo_st *uinfo = NULL;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	uinfo = task->uinfo;

	pthread_mutex_lock(&uinfo->user_task.task_list_lock);
	tinfo_put(task);	//release nref of taskid2taskinfo() hold
	ret = __apx_task_release(task);
	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);

	return ret;
}

int apx_task_destroy(int taskid)
{
	struct apx_task_st *task = NULL;
	int ret;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	/* delete task node from usertask list first,so the other thread couldn't find it again */
	utask_list_del(task->uinfo, task);
	tinfo_put(task);	//release nref of taskid2taskinfo() hold

	/* delete downloaded file */
	ret = __task_trans_release(task, 1);
        if (ret != 0)
                apx_wlog("delete task from disk failed", ret);

	/* wait other thread release task node */
	while (task->nref != 0)
		usleep(100000);

	/*  delete task from disk  */
	ret = tinfo_delete(task);
        if (ret != 0)
                apx_wlog("delete task from disk failed", ret);

	taskinfo_free(task);

	return ret;
}

int apx_task_remove(int taskid)
{
	struct apx_task_st *task = NULL;
	int ret;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	/* delete task node from usertask list first,so the other thread couldn't find it again */
	utask_list_del(task->uinfo, task);
	tinfo_put(task);	//release nref of taskid2taskinfo() hold

	/* delete downloaded file */
	ret = __task_trans_release(task, 1);

	/* wait other thread release task node */
	while (task->nref != 0)
		usleep(100000);

	/*  delete task from disk  */
	ret = tinfo_delete(task);
	taskinfo_free(task);
	return ret;
}

int apx_task_uri_check(struct apx_trans_opt *trans_opt)
{
	struct apx_userinfo_st *userinfo = NULL;
	int ret = 0;
	struct apx_task_st *p = NULL;
	int mode;

	/* parse uri */
	if (trans_opt == NULL)
		return -1;

	userinfo = uid2uinfo(current_uid_get());
	if (userinfo == NULL)
		return -2;

        trans_opt->proto = task_proto_from_uri(trans_opt->uri);
        if (trans_opt->proto == APX_TASK_PROTO_UNKNOWN)
                return -3;

	pthread_mutex_lock(&userinfo->user_task.task_list_lock);
	for (mode = APX_TASK_STATE_START; mode < APX_TASK_STATE_END; mode++)
	{

		list_for_each_entry(p, &userinfo->user_task.task_list[mode], list)
			if (p->trans_opt.uri && strncmp(trans_opt->uri, p->trans_opt.uri, strlen(trans_opt->uri)) == 0)
			{//modify by luyue
				ret = p->taskid;
				pthread_mutex_unlock(&userinfo->user_task.task_list_lock);
				return ret;
			}
	}
	pthread_mutex_unlock(&userinfo->user_task.task_list_lock);

	return g_trans_op[trans_opt->proto]->uri_check(trans_opt);
}

int apx_task_create(struct apx_trans_opt *trans_opt)
{
	int ret = 0;
	struct apx_userinfo_st	*uinfo = NULL;
	struct apx_task_st *task = NULL;
	//char * ptr;

	uinfo = uid2uinfo(current_uid_get());
	if (uinfo == NULL)
		return -1;

	/* parse uri */
	if (trans_opt == NULL)
		return -2;

	trans_opt->proto = task_proto_from_uri(trans_opt->uri);
	if (trans_opt->proto == APX_TASK_PROTO_UNKNOWN)
		return -3;

	/* init taskinfo */
	task = taskinfo_alloc();
	if (task == NULL)
		return -4;

	task->trans_opration = g_trans_op[trans_opt->proto];

	memcpy(&task->trans_opt, trans_opt, sizeof(struct apx_trans_opt));
	task->uinfo 		=	uinfo;
	task->taskid		=	trans_opt->taskid;

	task->create_time	=	time(NULL);
	ret = gettimeofday(&task->tick_time, NULL);
	if (ret == -1)
		//TODO? Sould I care it?
		;

	task->trans_handle = task->trans_opration->create();
          if (task->trans_handle == -1)
          {
		ret = -6;
		goto  err_out1;
	}

	/* end of init task info */
	utask_list_add(uinfo, task, APX_TASK_STATE_STOP);
	return task->taskid;
err_out1:
	taskinfo_free(task);
	return ret;
}

int apx_task_delete(int taskid)
{
	struct apx_task_st *task = NULL;
	int ret = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	ret = task2bedel(task);

	tinfo_put(task);

	return ret;
}

int apx_task_stop(int taskid)
{
	struct apx_task_st *task = NULL;
	int ret = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
	{
		 apx_wlog("Can't find taskid", taskid);
		return -1;
	}

	ret = task2stop(task);

	tinfo_put(task);

	return ret;
}

int apx_task_recover(int taskid)
{
	//struct apx_userinfo_st	*uinfo = NULL;
	struct apx_task_st *task = NULL;
	int ret = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	if (task->trans_stat.state == APX_TASK_STATE_FINTOBEDEL)
		utask_list_move(task->uinfo, task, APX_TASK_STATE_FINISHED);
	else
		ret =apx_task_stop(taskid);

	tinfo_put(task);
	return ret;
}


int apx_task_start(int taskid)
{
	struct apx_task_st *task = NULL;
	int ret = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	ret = task2wait(task);
	tinfo_put(task);
    return ret;
}

int apx_task_exist(int taskid, struct apx_trans_opt **trans_opt, struct apx_trans_stat **trans_stat)
{
	int i = 0;
	int ret = -1;
	struct apx_task_st * task = NULL;
	struct apx_userinfo_st *uinfo = NULL;

	uinfo = apx_conf_uinfo_get();
	if (uinfo == NULL)
		return ret;

	pthread_mutex_lock(&uinfo->user_task.task_list_lock);

	/* traverse three task list of the user to find the  corresponding task by taskid */
	for (i = APX_TASK_STATE_START; i < APX_TASK_STATE_END; i++)
	{
		list_for_each_entry(task, &uinfo->user_task.task_list[i], list)
		{
			if (task->taskid == (unsigned long long)taskid) {
				(*trans_opt) = &task->trans_opt;
				(*trans_stat) = &task->trans_stat;
				ret = 0;
				break;
			}
		}
	}

	pthread_mutex_unlock(&uinfo->user_task.task_list_lock);
	return ret;
}

int apx_task_reset(int taskid)
{
	int ret = 0;
	struct 	apx_trans_opt trans_opt = {0};
	struct apx_task_st *task = NULL;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	memcpy(&trans_opt, &task->trans_opt, sizeof(struct apx_trans_opt));

	tinfo_put(task);//release nref of taskid2taskinfo() hold

	ret = apx_task_destroy(taskid);
	if (ret)
	{
		apx_wlog("Delete old task failed, task reset failed", ret);
		return -2;
	}

	ret = apx_task_create(&trans_opt);
	if (ret)
	{
		 apx_wlog("Create new task failed, task reset failed", ret);
		return -3;
	}

	ret = apx_task_start(ret);
        if (ret)
                apx_wlog("task restart failed, task reset failed", ret);

	return ret;
}

int apx_task_file_name_get(int taskid, char*fpath, int path_lenth, char *fname, int name_lenth)
{
	//struct apx_userinfo_st	*uinfo = NULL;
	struct apx_task_st *task = NULL;
	//int ret = 0;

	if (fpath == NULL || path_lenth == 0 || fname == NULL || name_lenth == 0)
		return -1;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	memset(fpath, 0, path_lenth);
	memset(fname, 0, name_lenth);

	strncpy(fpath, task->trans_opt.fpath, path_lenth - 1);
	strncpy(fname, task->trans_opt.fname, name_lenth - 1);
	tinfo_put(task);
	return 0;
}

int apx_task_priv_get(int taskid)
{
	struct apx_task_st *task = NULL;
	u8 priv;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	priv = task->trans_opt.priv;
	tinfo_put(task);

	return priv;
}

int apx_task_priv_set(int taskid, u8 action, u8 task_priv)
{
	struct apx_task_st *task = NULL;
	//u8 priv;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	pthread_mutex_lock(&task->uinfo->user_task.task_list_lock);
	__utask_list_del(task->uinfo, task);

	switch(action)
	{
		case APX_TASK_PRIV_UP:
			if (task->trans_opt.priv != 0)
				task->trans_opt.priv--;
			break;

		case APX_TASK_PRIV_DOWN:
			if (task->trans_opt.priv != 0xFF)
				task->trans_opt.priv++;
			break;

		case APX_TASK_PRIV_TOP:
			task->trans_opt.priv = 0;
			break;

		case APX_TASK_PRIV_BOTTOM:
			task->trans_opt.priv = 0xFF;
			break;

		case APX_TASK_PRIV_SET:
			task->trans_opt.priv = task_priv;
			break;

		default:
			break;
	}

	__utask_list_add(task->uinfo, task, task->trans_stat.state);
	pthread_mutex_unlock(&task->uinfo->user_task.task_list_lock);

	tinfo_put(task);

	return 0;
}

int apx_task_limit_set(int taskid, u32 down_splimit)
{
	struct apx_task_st *task = NULL;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	task->trans_opt.down_splimit	=	down_splimit;
	//todo: should be supported by transfer_a2 & transfer_cl

	tinfo_put(task);

	return 0;
}

int apx_task_limit_get(int taskid, u32* down_splimit)
{
	struct apx_task_st *task = NULL;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	if (down_splimit != NULL)
		*down_splimit	=	task->trans_opt.down_splimit;


	tinfo_put(task);

	return 0;
}

int apx_task_ftp_account_set (int taskid, char* ftp_user, char* ftp_passwd)
{
	struct apx_task_st *task = NULL;

	if (ftp_user == NULL || ftp_passwd == NULL)
		return -1;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -2;

	strncpy(task->trans_opt.ftp_user, ftp_user, sizeof(task->trans_opt.ftp_user) - 1);
	strncpy(task->trans_opt.ftp_passwd, ftp_passwd, sizeof(task->trans_opt.ftp_passwd) - 1);

	tinfo_put(task);

	return 0;
}

int apx_task_concur_set (int taskid, u8 concur_num)
{
	//todo: should be supported by transfer_a2 & transfer_cl
	return 0;
}

int apx_task_concur_get (int taskid)
{
	struct apx_task_st *task = NULL;
	u16 concur = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	concur = task->trans_opt.concurr; //? task->trans_stat.connections
	tinfo_put(task);

	return concur;
}

int apx_task_speed_get(int taskid, u32 *down_sp, u32 *up_sp)
{
	struct apx_task_st *task = NULL;
	//int ret;
	struct apx_trans_stat *trans_stat = NULL;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	trans_stat = &task->trans_stat;

	if (down_sp != NULL)
		*down_sp =	trans_stat->down_speed;
	if (up_sp != NULL)
		*up_sp	=	trans_stat->up_speed;

	tinfo_put(task);

	return 0;
}

int apx_task_time_get(int taskid,
				time_t *create_time,
				time_t *start_time,
				time_t *last_start_time,
				time_t *last_stop_time,
				time_t *during_time)
{
	struct apx_task_st *task = NULL;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	if (create_time != NULL)
		*create_time	=	task->create_time;

	if (start_time != NULL)
		*start_time		=	task->start_time;

	if (last_start_time != NULL)
		*last_start_time =	task->last_start_time;

	if (last_stop_time != NULL)
		*last_stop_time	=	task->end_time;

	if (during_time != NULL)
	{
		if (task->trans_stat.state == APX_TASK_STATE_ACTIVE)
			*during_time = task->during_time + (time(NULL) - task->start_time);
		else
			*during_time	=	task->during_time;
	}

	tinfo_put(task);

	return 0;
}

int apx_task_proto_get(int taskid)
{
	struct apx_task_st *task = NULL;
	u8 proto = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	proto = task->trans_opt.proto;
	tinfo_put(task);

	return proto;
}

int apx_task_type_get(int taskid)
{
	struct apx_task_st *task = NULL;
	u8 type = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	type = task->trans_opt.type;
	tinfo_put(task);

	return type;
}

int apx_task_bpcontinue_get(int taskid)
{
	struct apx_task_st *task = NULL;
	u8 iscontinue = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	iscontinue = task->trans_opt.bp_continue;
	tinfo_put(task);

	return iscontinue;
}

int apx_task_state_get(int taskid)
{
	struct apx_task_st *task = NULL;
	u8 state = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	state = task->trans_stat.state;
	tinfo_put(task);
	return state;
}

int apx_task_uid_get(int taskid)
{
	struct apx_task_st *task = NULL;
	int uid = 0;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	uid = task->uinfo->userid;
	tinfo_put(task);

	return uid;
}

int apx_task_uri_get(int taskid, char* uri, u32 uri_lenth)
{
	struct apx_task_st *task = NULL;

	if (uri == NULL || uri_lenth == 0)
		return -1;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -2;

	strncpy(uri, task->trans_opt.uri, uri_lenth - 1);
	tinfo_put(task);

	return 0;
}

int apx_task_file_size_get(int taskid, u64 *total_size, u64 *local_size, u64* up_size)
{
	struct apx_task_st *task = NULL;
	//int ret = 0;
	struct apx_trans_stat *stat;

	task = taskid2taskinfo(taskid);
	if (task == NULL)
		return -1;

	stat = &task->trans_stat;
	/* contact with uri to get the file size which will be download */

	if (local_size != NULL)
		*local_size = stat->down_size;

	if (up_size != NULL)
		*up_size = stat->up_size;

	if (total_size != NULL)
	{
		*total_size = stat->total_size;
		if(*total_size==0)
		{
			tinfo_put(task);
            //有些请求服务器无法返回文件的总大小,例如cbs,认为是正常情况
            return 0;
			//return -2;
		}
	}

	tinfo_put(task);

	return 0;
}

int apx_task_btfile_get(int taskid, struct btfile* bt_file)
{
        struct apx_task_st *task = NULL;
        int ret = 0;

        if (bt_file == NULL)
                return -1;

        task = taskid2taskinfo(taskid);
        if (task == NULL)
                return -2;

        if (task->trans_opt.proto != APX_TASK_PROTO_BT)
        {
                ret = -3;
                goto out;
        }

        ret = task->trans_opration->get_btfile(task->trans_handle, &task->trans_opt, bt_file);

out:
        tinfo_put(task);
        return ret;
}

int apx_task_btfile_selected(int taskid, char* bt_selected)
{
        struct apx_task_st *task = NULL;
        int ret = 0;

        if (bt_selected == NULL)
                return -1;

        task = taskid2taskinfo(taskid);
        if (task == NULL)
                return -2;

        if (strlen(bt_selected) > sizeof(task->trans_opt.bt_select) - 1)
        {
                ret = -3;
                goto out;
        }

        strncpy(task->trans_opt.bt_select, bt_selected, sizeof(task->trans_opt.bt_select) - 1);

out:
        tinfo_put(task);
        return ret;
}

int apx_task_btfile_parse(char* bt_uri, char* bt_fname, int bt_fname_len)
{
	return apx_trans_parse_btfile_a2(bt_uri, bt_fname, bt_fname_len);
}

int apx_task_init()
{
	int ret = 0, i;
	apx_trans_global_init();

	g_trans_op[APX_TASK_PROTO_FTP]	= &g_trans_cl_op;
	g_trans_op[APX_TASK_PROTO_HTTP] = &g_trans_cl_op;
	g_trans_op[APX_TASK_PROTO_HTTPS] = &g_trans_cl_op;
    g_trans_op[APX_TASK_PROTO_BT]   = &g_trans_a2_op;
    //g_trans_op[APX_TASK_PROTO_DATA] = &g_trans_a2_op;

	for (i = APX_TASK_PROTO_START; i < APX_TASK_PROTO_END; i++)
	{
		ret |= g_trans_op[i]->init(0);
	}

	if (ret != 0)
	{
		for (i = APX_TASK_PROTO_START; i < APX_TASK_PROTO_END; i++)
		{
			g_trans_op[i]->exit();
		}
		apx_wlog_quit("task module init failed", ret);
	}
	apx_wlog("task inited", 0);
	return 0;
}

void apx_task_exit()
{
	int ret = 0, i = 0;
	ret = apx_user_task_traverse_all(current_uid_get(), __apx_task_release_save);
	for (i = APX_TASK_PROTO_START; i < APX_TASK_PROTO_END; i++)
	{
		g_trans_op[i]->exit();
	}

        apx_wlog("task exited", 0);
	apx_trans_global_cleanup();
}

