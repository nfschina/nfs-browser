	   
#include "content/browser/download/speed_download/client/linux/apx_user.h"
#include "content/browser/download/speed_download/client/linux/apx_config.h"
#include "content/browser/download/speed_download/client/linux/apx_wlog.h"

static struct apx_userinfo_st * userinfo_alloc(void)
{
        int i = 0;

        struct apx_userinfo_st *userinfo = NULL;
        userinfo = (struct apx_userinfo_st *)malloc(sizeof(struct apx_userinfo_st));
        if (userinfo == NULL)
                return NULL;

        memset(userinfo, 0, sizeof(struct apx_userinfo_st));

        pthread_mutex_init(&(userinfo->user_task.task_list_lock), NULL);

        INIT_LIST_HEAD(&userinfo->list);

        for (i = 0; i < APX_TASK_STATE_END; i++)
                INIT_LIST_HEAD(&userinfo->user_task.task_list[i]);

        return userinfo;
}

static void userinfo_free(struct apx_userinfo_st *uinfo)
{
        if (uinfo == NULL)
                return ;

        //if (uinfo->file_list) 
        //      free(uinfo->file_list);

        pthread_mutex_destroy(&(uinfo->user_task.task_list_lock));
        free(uinfo);
        return ;
}

int current_uid_get()
{
	struct apx_userinfo_st *uinfo = NULL;

	uinfo = apx_conf_uinfo_get();
	if (uinfo == NULL)
		return -1;
	else
		return uinfo->userid;
}

struct apx_userinfo_st *uid2uinfo(u32 uid)
{
	struct apx_userinfo_st *uinfo = NULL;
	uinfo = apx_conf_uinfo_get();
	if (uinfo == NULL)
		return NULL;
	if (uid == uinfo->userid)
	 	return uinfo;
	return NULL;
}

int apx_user_login()
{
        int ret = 0;    //login get user id
        struct apx_userinfo_st *uinfo = NULL;
        //int user_id_flag = 0;

        uinfo = apx_conf_uinfo_get();
        if (uinfo == NULL)
        {
                uinfo = userinfo_alloc();
                if (uinfo == NULL)
                        return -1;

                ret = apx_conf_uinfo_set(uinfo);
                if (ret < 0)
                {
                        userinfo_free(uinfo);
                        return -2;
                }
        }
        else if (uinfo->userid != 0)
        {
                printf("already login\n");
                return -3;
        }

        if (uinfo->checker_tid == 0)
        {
                ret = pthread_create(&uinfo->checker_tid, NULL, (void *)thread_utask_scheduler, uinfo);
                if(ret != 0)
                {
                        apx_wlog("utask checker thread create error",errno);
                        goto err_out;
                }
        }

       return uinfo->userid;

err_out:
        userinfo_free(uinfo);
        return -4;
}

/*int apx_user_limit_set(u32 uid, u32 down_speed, u32 up_speed, u32 task_num)
{
	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -1;

	userinfo->down_splimit = down_speed;
//	userinfo->up_splimit = up_speed;
	userinfo->active_task_limit = task_num;

	return 0;
}

int apx_user_limit_get(u32 uid, u32 *down_speed_limit, u32 *up_speed_limit, u32 *task_num_limit)
{
	struct apx_userinfo_st *userinfo = NULL;
	if (down_speed_limit == NULL || up_speed_limit == NULL || task_num_limit == NULL)
		return -1;

	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;
	
	*down_speed_limit = userinfo->down_splimit;
//	*up_speed_limit = userinfo->up_splimit;
	*task_num_limit = userinfo->active_task_limit;

	return 0;
}

int apx_user_task_num_get(u32 uid, u16 *active_task_num, u16 *stop_task_num, u16 *finished_task_num)
{
	if (active_task_num == NULL || stop_task_num == NULL || finished_task_num == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;

	*active_task_num = userinfo->user_task.task_num[APX_TASK_STATE_ACTIVE];
	*stop_task_num = userinfo->user_task.task_num[APX_TASK_STATE_STOP];
	*finished_task_num = userinfo->user_task.task_num[APX_TASK_STATE_FINISHED];

	return 0;
}

int apx_user_file_path_set(u32 uid, char *path)
{
	if (path == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;
	
	strncpy(userinfo->user_dir, path, sizeof(userinfo->user_dir) - 1);
	return 0;
}

int apx_user_file_path_get(u32 uid, char *path, int path_lenth)
{
	if (path == NULL)
		return -2;

	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -1;

	strncpy(path, userinfo->user_dir, path_lenth - 1);
	return 0;
}

int apx_user_login_time_get(u32 uid, time_t *login_time)
{
	if (login_time == NULL)
		return -1;
	
	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;

	*login_time = userinfo->login_time;
	return 0;
}

int apx_user_register_time_get(u32 uid, time_t *register_time)
{
	if (register_time == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;

	*register_time = userinfo->register_time;
	return 0;
}

int apx_user_last_login_time_get(u32 uid, time_t *last_login_time)
{
	if (last_login_time == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;

	*last_login_time = userinfo->last_login_time;
	return 0;
}

int __apx_user_task_traverse(u32 uid, u8 mode, void (*func)(void *data))
{
	if (func == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	struct apx_task_st *p = NULL;

	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;
	
	if(list_empty(&userinfo->user_task.task_list[mode]))
		return -3;

	list_for_each_entry( p, &userinfo->user_task.task_list[mode], list)
		func((void *)(p));
	
	return 0;
}

int apx_user_task_traverse(u32 uid, u8 mode, void (*func)(void *data))
{
	if (func == NULL)
		return -1;

	struct apx_userinfo_st *userinfo = NULL;
	int ret = 0;


	userinfo = uid2uinfo(uid);
	if (userinfo == NULL)
		return -2;	
	
    pthread_mutex_lock(&userinfo->user_task.task_list_lock);
	ret = __apx_user_task_traverse(uid, mode, func);
    pthread_mutex_unlock(&userinfo->user_task.task_list_lock);
	
	return ret;
}
*/

