#ifndef APX_HFTSC_API_H_
#define APX_HFTSC_API_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <semaphore.h>
#else
#include <windows.h>
#include <winsock.h>
#include <time.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include<direct.h>
#include <stdint.h>
#endif

#ifndef SPEED_IMPLEMENTATION
#define SPEED_DOWNLOAD_EXTERN
#elif defined _WIN32 || defined __GYGWIN__
#define SPEED_DOWNLOAD_EXTERN __declspec(dllexport)
#else
#define SPEED_DOWNLOAD_EXTERN __attribute__ ((visibility ("default")))
#endif

#define HTTP_OK         ( 200 )
#define HTTP_206        ( 206 ) /** Partial Content */
#define HTTP_403        ( 403 ) /** Forbidden */
#define HTTP_404        ( 404 ) /** Not Found */
#define HTTP_410        ( 410 ) /** Gone */
#define HTTP_412        ( 412 ) /** Precondition Failed */

#define APX_SIGN_LEN    ( 41 )
#define APX_ID_LEN      ( 64 )
#define BUF_LEN_MAX     ( 128 )
#define URL_LEN_MAX     ( 512 )

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

struct apx_trans_globalstat
{
        int             downspeed;
        int             uploadspeed;
        int             activenum;
        int             waitingnum;
        int             stoppednum;
};

struct apx_trans_glboptions
{
        char            path[256];
        int             connections;
        int             max_limit_downspeed;
        int             max_limit_uploadspeed;
        int             max_concurrent_download;
};

struct apx_trans_stat
{
        u64             total_size;
        u64             down_size;
        u64             up_size;
        u32             down_speed;
        u32             up_speed;
        u16             connections;
        u8              state;
        u8              state_event;
        int             trans_errno;
        int             disk_flag;
};

struct btfile
{
        char fn[512];
        char file[40][512];
        int  size;
};

struct apx_trans_opt
{
	u8	type; 
        u64     taskid;
	u8	priv;
	u8	proto;
	u8      concurr;
	u32	down_splimit;
        u32     up_splimit;
	char ftp_user[256];
	char ftp_passwd[256];
	char fpath[256];
	char fname[256];
        char bt_select[256];
	char* uri;
	u64 fsize;
	u8 bp_continue;
	u8   keep_close; /** Connection: close */
	u8   checksum; /** check file sha1sum while downloading */
	u8   isgzip;   /**open gzip or not, 0-close 1-open */
	char *cookie;
	char *referer;
        char *method;
        char *pack_url;
	char if_match[256];
};

/* DEFINE TASK PRIVILEGE OPERATION */
#define APX_TASK_PRIV_UP	 0  
#define APX_TASK_PRIV_DOWN	 1  
#define APX_TASK_PRIV_TOP	 2 
#define APX_TASK_PRIV_BOTTOM  3  
#define APX_TASK_PRIV_SET 	4

/* DEFINE TASK STATE */
#define APX_TASK_STATE_UNDEFINED  0
#define APX_TASK_STATE_START  1

#define APX_TASK_STATE_ACTIVE   1
#define APX_TASK_STATE_WAIT   	2
#define APX_TASK_STATE_STOP     3
#define APX_TASK_STATE_TOBEDEL  4
#define APX_TASK_STATE_FINTOBEDEL	5
#define APX_TASK_STATE_FINISHED		6
#define APX_TASK_STATE_TERMINATED	7

#define APX_TASK_STATE_END   	8
#define APX_TASK_STATE_CREATE   APX_TASK_STATE_STOP

/* DEFINE TASK TYPE */
#define APX_TASK_TYPE_UNKNOWN  0
#define APX_TASK_TYPE_DOWN     1
#define APX_TASK_TYPE_SERVER_DOWN 2
#define APX_TASK_TYPE_SERVER_UP   3

/* DEFINE TASK PROTO */
#define APX_TASK_PROTO_UNKNOWN  0
#define APX_TASK_PROTO_START  1

#define APX_TASK_PROTO_FTP	1
#define APX_TASK_PROTO_HTTP  2
#define APX_TASK_PROTO_HTTPS  3
#define APX_TASK_PROTO_BT   4

//#define APX_TASK_PROTO_DATA 5
#define APX_TASK_PROTO_END   5

int apx_task_restore(int uid);

/*	check uri, trans_opt should be filled with uri/ftp_user/ftp_passwd. 
	when function return, trans_opt->fname/fsize/bp_continue will be set.
	If uri bad, return < 0 										*/
int apx_task_uri_check(struct apx_trans_opt *trans_opt);
#if !defined(_WIN32)
int apx_task_init();
#else
void apx_task_init();
#endif
void apx_task_exit();

int apx_task_exist(int taskid, struct apx_trans_opt **trans_opt, struct apx_trans_stat **trans_stat);
int apx_task_create(struct apx_trans_opt *trans_opt);
int apx_task_destroy(int taskid);
int apx_task_remove(int taskid);
int apx_task_release(int taskid);
int apx_task_stop(int taskid);
int apx_task_start(int taskid);
int apx_task_delete(int taskid);
int apx_task_recover(int taskid);
int apx_task_reset(int taskid);
int apx_task_file_name_get(int taskid, char *fpath, int path_lenth, char* fname, int name_lenth);
int apx_task_priv_get(int taskid);
int apx_task_limit_set(int taskid, u32 down_splimit);
int apx_task_limit_get(int taskid, u32* down_splimit);
int apx_task_ftp_account_set (int taskid, char* ftp_user, char* ftp_passwd);
int apx_task_concur_get (int taskid);
int apx_task_speed_get(int taskid, u32 *down_sp, u32 *up_sp);
int apx_task_time_get(int taskid, time_t *create_time, time_t *start_time, time_t *last_start_time, time_t *last_stop_time, time_t * during_time);
int apx_task_proto_get(int taskid);
int apx_task_type_get(int taskid);
int apx_task_state_get(int taskid);
int apx_task_uid_get(int taskid);
int apx_task_uri_get(int taskid, char* uri, u32 uri_lenth);
int apx_task_file_size_get(int taskid, u64 *total_size, u64 *local_size, u64* up_size);
int apx_task_btfile_get(int taskid, struct btfile* bt_file);
int apx_task_btfile_selected(int taskid, char* bt_selected);
int apx_task_btfile_parse(char* bt_uri, char* bt_fname, int bt_fname_len);

/* CONFIGURATE MANAGMENT MODULE API */
void apx_conf_init(char *fpath_conf);
int apx_conf_writeback(void);
int apx_conf_release(void);
//int apx_conf_active_task_limit_set(int task_num);
//int apx_conf_active_task_limit_get(void);
//int apx_conf_log_set(char *logfile);
int apx_conf_log_get(char *logfile, int logfile_size);
struct apx_userinfo_st * apx_conf_uinfo_get(void);
int apx_conf_uinfo_set(struct apx_userinfo_st * userinfo);
//int apx_conf_nextid_get(void);
//int apx_conf_nextid_inc(void);
int apx_user_login();
s32 apx_file_is_exist( s8 *path, u64 *pSize );
s32 apx_file_mkdir( s8* path );

typedef enum _apx_network_e_
{
        APX_NET_OK = 0,
        APX_NET_INTER_ERR,
        APX_NET_IP_UNSET,
        APX_NET_ROUTE_UNSET,
        APX_NET_ROUTE_UNREACH,
        APX_NET_DNS_UNSET,
        APX_NET_DNS_UNREACH,
        APX_NET_UNKOWN,

        APX_NET_MAX
}APX_NETWORK_E;

void apx_net_start( void );
void apx_net_end( void );
APX_NETWORK_E apx_net_detect_interface( void );
APX_NETWORK_E apx_net_detect_ip( void );
APX_NETWORK_E apx_net_detect_route( char *pRt, char *pDst );

/* GLOBAL API */
SPEED_DOWNLOAD_EXTERN void apx_hftsc_init(char *fpath_conf);
SPEED_DOWNLOAD_EXTERN void apx_hftsc_exit();

#endif

