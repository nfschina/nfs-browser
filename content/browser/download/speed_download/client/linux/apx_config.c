
#include <sys/types.h>
#include <sys/stat.h>

#include "content/browser/download/speed_download/client/linux/apx_config.h"
#include "content/browser/download/speed_download/client/linux/apx_wlog.h"

// define the download use data
char DEFAULT_CONFIG[128] = {0};
char TASK_INFO[128] = {0};

struct apx_config_st g_config;

/*static struct apx_config_st *conf_alloc( void )
{
    struct apx_config_st * conf;

    conf = (struct apx_config_st *)malloc(sizeof(struct apx_config_st));
    if (!conf)
        return NULL;

    memset(conf, 0, sizeof(struct apx_config_st));

    return conf;
}

static void conf_free(struct apx_config_st *conf)
{

    if (conf == NULL)
        return ;

    free(conf);
    return;
}*/

int parse_conf_file(
        char conf_file[],
        const unsigned int conf_len,
        char name[],
        const unsigned int name_len,
        char path[],
        const unsigned int path_len)
{
    char *p = NULL;
    char conf_path[128] = {0};;

    if (conf_file == NULL || name == NULL || path == NULL ||
            conf_len == 0 || name_len == 0 || path_len == 0)
        return -1;

    memset(name, 0, name_len);
    memset(path, 0, path_len);

    if (conf_file[0] == '~')
    {
        strncpy(conf_path, getenv("HOME"), sizeof(conf_path) - 1);
        strncat(conf_path, conf_file+1, sizeof(conf_path) - strlen(conf_path) - 1);
        strncpy(conf_file, conf_path, conf_len - 1);
    }

    p = strrchr(conf_file, '/');
    if (p != NULL)
    {
        strncpy(name, p + 1, name_len - 1);
        strncat(path, conf_file, strlen(conf_file) - strlen(name) - 1);
    }
    else
    {
        strncpy(name, conf_file, name_len - 1);
        strncpy(path, "./", path_len - 1);
    }
    if (access(path, (R_OK | W_OK)))
        return -2;
    if (access(conf_file, (R_OK | W_OK)))
        return -3;

    return 0;
}

static int apx_mkdirs(char *path)
{
    char str[256] = {0};
    char *p = NULL;

    if (path == NULL)
        return -1;

    strncpy(str, path, sizeof(str) - 1);
    if ((strncmp(str, ".", sizeof(str)) == 0) || (strncmp(str, "/", sizeof(str)) == 0))
        return 1;

    if (access(str, F_OK) == 0)
    {
        if (access(str, (R_OK | W_OK) == -1))
            if (chmod(str, 0666) == -1)
                return -2;
        return 2;
    }

    p = strrchr(str, '/');
    if (p == NULL)
    {
        if (mkdir(str, 0777) < 0)
            return 0;
    }
    else
    {
        *p = '\0';
        if (apx_mkdirs(str) < -1)
            return -3;
        else
        {
            printf("mkdir %s\n", path);
            if (mkdir(path, 0777) < 0)
                return -4;
        }
    }
    return 0;
}

void apx_conf_init(char *fpath_conf)
{
    struct uci_package *pkg = NULL;
    struct uci_context *ctx = NULL;
    struct uci_ptr ptr;
    char file_name[128] = {0};
    char file_path[128] = {0};
    char buf[128] = {0};
    //int ret = 0;
    int res = 0;

    if (g_config.conf_init_flag)
    {
        printf("already init\n");
        return;
    }

    strncpy(DEFAULT_CONFIG, fpath_conf, sizeof(DEFAULT_CONFIG) - 1);
    strncat(DEFAULT_CONFIG, "/conf_file", sizeof(DEFAULT_CONFIG) - strlen(DEFAULT_CONFIG)- 1);
    strncpy(TASK_INFO, fpath_conf, sizeof(TASK_INFO) - 1);
    strncat(TASK_INFO, "/taskinfo", sizeof(TASK_INFO) - strlen(TASK_INFO)- 1);

    memset(&ptr, 0, sizeof(struct uci_ptr));
    strncpy(g_config.config_file, DEFAULT_CONFIG, sizeof(g_config.config_file) - 1);

    strncpy(g_config.log_file, fpath_conf, sizeof(g_config.log_file) - 1);
    strncat(g_config.log_file, "/hfts.log", sizeof(g_config.log_file) - strlen(g_config.log_file)- 1);

    res = parse_conf_file(g_config.config_file, sizeof(g_config.config_file),
            file_name, sizeof(file_name), file_path, sizeof(file_path));
    if (res)
    {
        g_config.active_task_limit = DEFAULT_ACT_TASK;
        g_config.next_taskid = DEFAULT_NEXT_TASKID;
        g_config.conf_init_flag = 1;
        apx_conf_writeback();
        return;
    }

    ctx = uci_alloc_context(file_path);
    if (ctx == NULL)
        return;

    if (uci_set_confdir(ctx, file_path) != UCI_OK)
    {
        uci_free_context(ctx);
        return;
    }

    if (uci_load(ctx, file_name, &pkg) != UCI_OK)
    {
        uci_free_context(ctx);
        return;
    }

    /* get active task limit */
    snprintf(buf, sizeof(buf) - 1, "%s.task.ActLimitNum", file_name);
    if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
    {
        uci_perror(ctx, NULL);
        g_config.active_task_limit = DEFAULT_ACT_TASK;
        return;
    }

    g_config.active_task_limit = atoi(ptr.o->v.string);

    /* get next task id */
    snprintf(buf, sizeof(buf) - 1, "%s.task.NextId", file_name);
    if (uci_lookup_ptr(ctx, &ptr, buf, true) != UCI_OK)
    {
        uci_perror(ctx, NULL);
        g_config.next_taskid = DEFAULT_NEXT_TASKID;
        return;
    }

    g_config.next_taskid = atoll(ptr.o->v.string);
    g_config.conf_init_flag = 1;
}

int apx_conf_writeback(void)
{
    struct uci_package *pkg = NULL;
    struct uci_context *ctx = NULL;
    struct uci_ptr ptr;
    char file_name[128] = {0};
    char file_path[128] = {0};
    char buf[128] = {0};
    char p[24] = {0};
    int ret = 0;
    int fd = -1;

    if (!g_config.conf_init_flag)
    {
        printf("config not init\n");
        return -1;
    }

    int res = parse_conf_file(g_config.config_file, sizeof(g_config.config_file),
            file_name, sizeof(file_name), file_path, sizeof(file_path));
    if (res)
    {
        umask(0);
        if (res == -1)
        {
	   apx_wlog("parse_conf_file failed\n", res);
            return -2;
        }
        else if (res == -2)
        {
            printf("file_path = %s\n", file_path);
            if (apx_mkdirs(file_path) < 0)
                return -3;
        }

        if ((fd = creat(g_config.config_file, 0666)) == -1)
        {
	   apx_wlog("parse_conf_file failed\n", res);
            return -4;
        }
        else
            close(fd);
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

    if (uci_lookup_section(ctx, pkg, "task") == NULL)
    {
        snprintf(buf, sizeof(buf) - 1, "%s.task", file_name);
        uci_lookup_ptr(ctx, &ptr, buf, true);
        ptr.value = "global";
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
    }

    /* set task active limit */
    snprintf(buf, sizeof(buf) - 1, "%s.task.ActLimitNum", file_name);
    uci_lookup_ptr(ctx, &ptr, buf, true);

    snprintf(p, sizeof(p) - 1, "%d", g_config.active_task_limit);
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

    /* set task next id */
    snprintf(buf, sizeof(buf) - 1, "%s.task.NextId", file_name);
    uci_lookup_ptr(ctx, &ptr, buf, true);

    snprintf(p, sizeof(p) - 1, "%lld", g_config.next_taskid);
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

    if (uci_lookup_section(ctx, pkg, "hfts") == NULL)
    {
        snprintf(buf, sizeof(buf) - 1, "%s.hfts", file_name);
        uci_lookup_ptr(ctx, &ptr, buf, true);
        ptr.value = "global";
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
    }

    /* set log file */
    snprintf(buf, sizeof(buf) -1, "%s.hfts.LogFile", file_name);
    uci_lookup_ptr(ctx, &ptr, buf, true);

    ptr.value = g_config.log_file;
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

    if (uci_commit(ctx, &pkg, true) != UCI_OK)
    {
        uci_perror(ctx, NULL);
        ret = -26;
    }

out:
    uci_unload(ctx, pkg);
    uci_free_context(ctx);
    return  ret;
}

int apx_conf_release(void)
{
    return apx_conf_writeback();
}

int apx_conf_active_task_limit_set(int task_num)
{
    if (!g_config.conf_init_flag)
        return -1;

    g_config.active_task_limit = task_num;

    return apx_conf_writeback();
}

int apx_conf_active_task_limit_get(void)
{
    if (!g_config.conf_init_flag)
        return -1;

    return g_config.active_task_limit;
}

int apx_conf_log_get(char *logfile, int logfile_size)
{
    if (!g_config.conf_init_flag)
        return -1;
    if (logfile != NULL && logfile_size > 0)
    {
        if (g_config.log_file[0] == '~')
        {
            strncpy(logfile, getenv("HOME"), logfile_size - 1);
            strncat(logfile, &g_config.log_file[1], logfile_size - strlen(getenv("HOME"))- 1);
        }
        else
            strncpy(logfile, g_config.log_file, logfile_size - 1);
    }

    return 0;
}

int apx_conf_log_set(char *logfile)
{
    if (!g_config.conf_init_flag)
        return -1;

    //char path[128] = {0};
    if (logfile != NULL)
    {
        if (*logfile == '~')
        {
            strncpy(g_config.log_file, getenv("HOME"), sizeof(g_config.log_file) - 1);
            logfile++;
        }
        strncat(g_config.log_file, logfile, sizeof(g_config.log_file) - strlen(g_config.log_file)- 1);
    }

    return 0;
}

struct apx_userinfo_st *apx_conf_uinfo_get(void)
{
    if (!g_config.conf_init_flag)
        return NULL;

    return g_config.userinfo;
}

int apx_conf_uinfo_set(struct apx_userinfo_st *userinfo)
{
    if (userinfo == NULL)
        return -1;

    if (!g_config.conf_init_flag)
        return -2;

    g_config.userinfo = userinfo;
    return 0;
}

/*int apx_conf_nextid_get(void)
{
    if (!g_config.conf_init_flag)
        return -1;

    return g_config.next_taskid;
}

int apx_conf_nextid_inc(void)
{
    if (!g_config.conf_init_flag)
        return -1;

    return g_config.next_taskid++;
}*/

