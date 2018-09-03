#ifndef APX_TRANSFER_A2_H
#define APX_TRANSFER_A2_H
#ifndef _GUN_SOURCE
#define _GUN_SOURCE
#endif
#include <stdint.h>
#include <string.h>
#include <pthread.h>

//#include "content/browser/download/speed_download/include/apx_list.h"
#include "content/browser/download/speed_download/include/apx_hftsc_api.h"
#ifdef __cplusplus
extern "C"{
#endif

/**
        @brief handle struct
        handle is used to point aria2 session and thread..

        @param session                  mark aria2 session
        @param thread                   mark start session thread
        @param flag                     thread start or stop
        @param gid                      task id
        @param status                   task status
*/
struct handle
{
        u32 *session;
        pthread_t thread;
        int flag;
        uint64_t gid;
        int status;
        char fname[256];
};

int apx_trans_init_a2( u32 nu );
int apx_trans_recv_a2( u32 nu );
int apx_trans_stop_a2( u32 nu) ;
int apx_trans_getopt_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
int apx_trans_precreate_a2( struct apx_trans_opt* task_opt );
int apx_trans_create_a2( void );
int apx_trans_getstat_a2( u32 nu, struct apx_trans_stat* task_stat );
int apx_trans_start_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
int apx_trans_del_file_a2( struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
int apx_trans_delete_a2( u32 nu );
int apx_trans_get_btfile_a2( u32 nu, struct apx_trans_opt* task_opt, struct btfile* bt_file );
int apx_trans_parse_btfile_a2( char* bt_uri, char* bt_fname, int bt_fname_len );
int apx_trans_release_a2( u32 nu, int flags );
void apx_trans_exit_a2( void );
#ifdef __cplusplus
};
#endif

#endif

