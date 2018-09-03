#ifndef APX_FILE_H_20150210
#define APX_FILE_H_20150210
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#include <stddef.h>

/*---Include Local.h File---*/
#include "content/browser/download/speed_download/include/apx_list.h"
#include "content/browser/download/speed_download/include/apx_hftsc_api.h"
 
#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
#define	APX_FILE_BLK_FULL		( -0x1A1B1C1D )       
#define	APX_FILE_FULL			( -0x2A2B2C2D )      

/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
typedef	void	FILE_T;

typedef	enum	_apx_file_e_
{
	APX_FILE_SUCESS = 0,
	APX_FILE_DOWNLOADING = -5,	
	APX_FILE_DOWNLOADED,	
	APX_FILE_NO_MEM,		
	APX_FILE_PARAM_ERR, 
	APX_FILE_UNKOWN, 
	
	APX_FILE_MAX
}apx_file_e;


typedef	struct	_apx_fblock_s_
{
	u64	u64Start;		
	u64	u64End;			
	u64	u64Offset;	
	s32	s32Idx;
	u8	u8Proxy;
	u8	sign[41];
	char padding[8];
}apx_fblk_st;
        
void apx_file_init(void );

void apx_file_exit(void );

FILE_T* apx_file_create( s8 *ps8Url, s8 *ps8FName, u64 u64FSize,
							s32 s32BlkCnt, s32 *ps32Err, u32 u32Upload );

int apx_file_reset( FILE_T *pFileInfo );

void apx_file_release( FILE_T *pFileInfo );

void apx_file_destroy( FILE_T *pFileInfo);

ssize_t apx_file_read( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo );

ssize_t apx_file_write( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo );

u64 apx_file_size( FILE_T *pFileInfo );

u64 apx_file_cur_size( FILE_T *pFileInfo );

void apx_file_pause( FILE_T *pFileInfo );

void apx_file_resume( FILE_T *pFileInfo );

u64 apx_file_proxy_size( FILE_T *pFileInfo );
apx_fblk_st* apx_file_blk_info( FILE_T *pFileInfo );
void apx_file_blk_reset( FILE_T *pFileInfo, apx_fblk_st *pstblk );
s32 apx_file_blk_cnt( FILE_T *pFileInfo );
//s32 apx_file_is_exist( s8 *path, u64 *pSize );
//s32 apx_file_mkdir( s8* path );
s32 apx_file_divide_cnt( u64 u64Size, u32 upload );
void* apx_block_point( FILE_T *pFileInfo );
void apx_block_reset( FILE_T *pFileInfo );
void apx_block_set_status( void* pBlk, int index, int status );
void apx_file_set_cur_size( FILE_T *pFileInfo, u64 u64Size );
void apx_file_update_upsize( FILE_T *pFileInfo );
void apx_file_delete_all( s8* pFileName );

int apx_file_is_changed(FILE_T *pFileInfo, s8 *ps8FName, s8	*fname);

#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end APX_FILE_H_20150210 */       
/** @} */
 
