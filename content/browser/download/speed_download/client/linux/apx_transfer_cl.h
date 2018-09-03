#ifndef _APX_TRANSFER_CL_H_
#define _APX_TRANSFER_CL_H_
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif        
/*---Include Local.h File---*/
#include "content/browser/download/speed_download/include/apx_list.h"
#include "content/browser/download/speed_download/include/apx_hftsc_api.h"

#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
/**
*/

void apx_trans_global_init( void );
void apx_trans_global_cleanup( void );

int apx_trans_init_cl( u32 nu );

void apx_trans_exit_cl( void );

int apx_trans_create_cl( void );

int apx_trans_release_cl( u32 nu, int flags );

int apx_trans_start_cl( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );

int apx_trans_stop_cl( u32 nu);

int apx_trans_delete_cl( u32 nu );

int apx_trans_recv_cl( u32 nu );

int apx_trans_getopt_cl( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );

int apx_trans_getstat_cl( u32 nu, struct apx_trans_stat* task_stat );

int apx_trans_precreate_cl( struct apx_trans_opt* task_opt );

int apx_trans_del_file_cl( struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
         
#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end _APX_TRANSFER_CL_H_ */       
