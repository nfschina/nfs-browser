/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <zlib.h>

/*---Include Local.h File---*/
#include "content/browser/download/speed_download/client/linux/apx_file.h"

/*------------------------------------------------------------*/
/*                          Private Macros Defines                          */
/*------------------------------------------------------------*/
#define	BLOCK_MAX			( 256 )
#define	FILE_1K				( 1024 )
#define	FILE_1M				( 1024 * FILE_1K )
#define	FILE_10M			( 10 * FILE_1M )
#define	FILE_100M			( 10 * FILE_10M )
#define	FILE_1G				( 1024 * FILE_1M )

#define	FILE_TMP_SUFFIX		".tmp"
#define	FILE_TMP_UNGZIP		".ungzip"


#define CHUNK 65535      /*uzip buff*/

#define	FILE_INFO_SUFFIX		".info"

#define	FILE_LEN_MAX		( 1024 )

#define	free_func( _ptr_ )	\
do {							\
	if( _ptr_ != NULL ) {		\
		free( _ptr_ );			\
		_ptr_ = NULL;		\
	}						\
}							\
while(0)
	
#define fblock_free( _ptr_ )	free_func( _ptr_ )
#define finfo_free( _ptr_ )		free_func( _ptr_ )

#define HF_FILE_DEBUG

/*------------------------------------------------------------*/
/*                          Private Type Defines                            */
/*------------------------------------------------------------*/

typedef	enum	_blk_state_e_
{
	APX_BLK_EMPTY = 0,
	APX_BLK_FULL,	
	APX_BLK_HOLD,	
	APX_BLK_ALLOCED,
	APX_BLK_UNUSED,
	
	APX_BLK_MAX
}blk_state_e;


typedef	struct	_apx_fblk_s_
{
	u64	u64Start;		
	u64	u64End;			
	u64	u64Offset;	
	s32	s32Idx;
	u8	u8Proxy;
	u8	sign[41];
	s32	s32Fd;	
	blk_state_e	eState;	
}fblk_st;

typedef	struct _apx_fileInfo_s_
{
	s8	*ps8FName;	
	s8	*ps8Url;
	s8	*ps8BlkMap;
	u32	u32FNameMagic;
	u32	u32UrlMagic;
	u32	u32Upload;
	s32	s32Pause;
	u64	u64Size;	
	u64	u64CurSize;	
	u64	u64UpSize;	
	u64	u64ProxySize; 
	s32	s32BlkCnt;	
//	struct list_head stBlkHead;
	struct list_head	stList;
	pthread_mutex_t	mBlkLock;
	pthread_mutex_t	mInfoLock;
	fblk_st*	pstBlkInfo;
}finfo_st;


typedef struct _file_head_t_
{
	s32	s32Cnt;
	struct list_head stHeadList;
	pthread_rwlock_t rwLock;
}file_head_st;       
        
/*------------------------------------------------------------*/
/*                         Global Variables                                */
/*------------------------------------------------------------*/
/*---Extern Variables---*/
        
/*---Local Variables---*/
static s32 g_file_init;
static file_head_st g_file_head = {	.s32Cnt = 0,
							.stHeadList = LIST_HEAD_INIT( g_file_head.stHeadList ),
							.rwLock = PTHREAD_RWLOCK_INITIALIZER
							//.mInfoLock = PTHREAD_MUTEX_INITIALIZER
						};

/*------------------------------------------------------------*/
/*                          Local Function Prototypes                       */
/*------------------------------------------------------------*/
static u32 __string_magic( s8 *ps8Str );
static s32 __is_file_exist( s8 *ps8File, u64 *pSize );
static fblk_st* fblock_alloc( size_t nMemb  );
static void fblock_check_cnt( finfo_st *pstFileInfo );
static s32 fblock_new( finfo_st *pstFileInfo );
static void fblock_release( finfo_st *pstFileInfo );
static finfo_st* finfo_alloc( void  );
static finfo_st* finfo_new( s8 *ps8Url, s8 *ps8FName, u64 u64FSize );
static void finfo_release( finfo_st *pstFileInfo );
static finfo_st* __lookup_file_list( s8 *ps8Url, s8 *ps8FName );
static s32 __ftmp_create( finfo_st *pstFileInfo );
static s32 __fblock_open( fblk_st *pstBlkInfo, s8* ps8FileName, u32 upload );
static s32 __finfo_map( finfo_st *pstFileInfo, size_t sSize );
static void __finfo_unmap( finfo_st *pstFileInfo );
static s32 __finfo_parse( finfo_st *pstFileInfo, size_t sSize );
static void __finfo_update( finfo_st *pstFileInfo, s8 first );
static s32 __finfo_create( finfo_st *pstFileInfo );
static void __file_check_fname( finfo_st *pstFileInfo );
static s32 __file_create( finfo_st *pstFileInfo );
static s32 finfo_reload( finfo_st *pstFileInfo );
void apx_file_delete( s8* pFileName );

/*------------------------------------------------------------*/
/*                        Functions                                               */
/*------------------------------------------------------------*/

/**maigc num*/
#define MAGIC_SIZE        24 
static u32 __string_magic( s8 *ps8Str )
{
	u32 magic = 0;
	s32 pos;
	s8 inbuf[MAGIC_SIZE];

	if( NULL == ps8Str )
	{
		return 0;
	}
	
	memset( inbuf, 0, sizeof( inbuf ) );
	strncpy( ( char* )inbuf, ( char* )ps8Str, sizeof( inbuf ) );
	for( pos = 0; pos < MAGIC_SIZE; pos++ )
	{
		magic += ( u32 )( u8 )inbuf[pos];
		magic <<= 1;
	}
	return magic;
}

static s32 __is_file_exist( s8 *ps8File, u64 *pSize )
{
	s32 s32Ret = 0;
	struct stat stStat;

	s32Ret = stat( ( char* )ps8File, &stStat );
	if( pSize != NULL )
	{
		*pSize = stStat.st_size;
	}

	return ( 0 == s32Ret && S_ISREG( stStat.st_mode ) );
}

static fblk_st* fblock_alloc( size_t nMemb  )
{
	fblk_st *pstBlk = NULL;

	pstBlk = calloc( nMemb, sizeof( fblk_st ) );
	return ( NULL == pstBlk ) ? NULL : pstBlk;
}

s32 apx_file_divide_cnt( u64 u64Size, u32 upload )
{
	s32 s32BlkCnt = 1;
	u64 divide = 0;

	divide = u64Size / FILE_1M;
	if( 0 == u64Size )
	{
		s32BlkCnt = 1;
	}
	else if( divide < 3 )/** <= 3M */
	{
		s32BlkCnt = ( upload ) ? 1 : 1;
	}
	else if( divide < 5 )/** <=5M */
	{
		s32BlkCnt = ( upload ) ? 1 : 2;
	}
	else if( divide < 11 )/** <=10M */
	{
		s32BlkCnt = ( upload ) ? 2 : 3;
	}
	//最大线程数
	else if( divide < 51 )/** <=50M */
	{
		//s32BlkCnt = ( upload ) ? 5 : 20;
		s32BlkCnt = ( upload ) ? 5 : 5;
	}
	else if( divide < 101 )/** <=100M */
	{
		//s32BlkCnt = ( upload ) ? 10 : 50;
		s32BlkCnt = ( upload ) ? 5 : 5;
	}
	else if( divide < 501 )/** <= 500M */
	{
		//s32BlkCnt = ( upload ) ? 50 : 100;
		s32BlkCnt = ( upload ) ? 5 : 5;
	}
	else if( divide < 1025 )/** <=1024M 1G */
	{
		//s32BlkCnt = ( upload ) ? 100 : 200;
		s32BlkCnt = ( upload ) ? 5 : 5;
	}
	else
	{
		//s32BlkCnt = ( upload ) ? 150 :300;
		s32BlkCnt = ( upload ) ? 5 : 5;
	}	

	return s32BlkCnt;
}

static void fblock_check_cnt( finfo_st *pstFileInfo )
{	
	if( pstFileInfo->s32BlkCnt > 0 )
	{
		return;
	}

	pstFileInfo->s32BlkCnt = apx_file_divide_cnt( pstFileInfo->u64Size,
											pstFileInfo->u32Upload );
	return;
}

static s32 fblock_new( finfo_st *pstFileInfo )
{
	u64 i = 0;
	u64 s32BlkSize = 0;
	fblk_st *pstBlk = NULL;
			
	fblock_check_cnt( pstFileInfo ); 
	pstBlk = fblock_alloc( pstFileInfo->s32BlkCnt );
	if( NULL == pstBlk )
	{
		return -1;
	}

	s32BlkSize = pstFileInfo->u64Size / pstFileInfo->s32BlkCnt;
	for( i = 0; i < ( u64 )pstFileInfo->s32BlkCnt; i++ )
	{
		pstBlk[i].s32Fd= -1;
		pstBlk[i].s32Idx = i;
		pstBlk[i].u64Start = i * s32BlkSize;
		pstBlk[i].u64End =( s32BlkSize > 0 ) ? ( pstBlk[i].u64Start + s32BlkSize -1 ) : 0;//( i + 1 )* s32BlkSize -1;
	}

	pstBlk[i - 1].u64End = ( pstFileInfo->u64Size > 0 ) ? pstFileInfo->u64Size -1 : 0;
	pstFileInfo->pstBlkInfo = pstBlk;
	return 0;
}

static void fblock_close( finfo_st *pstFileInfo )
{
	s32 k = 0;
	fblk_st *pstBlk = pstFileInfo->pstBlkInfo;
	
	if( pstBlk != NULL )
	{
		pthread_mutex_lock( &pstFileInfo->mBlkLock );
		for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
		{
			if( pstBlk[k].s32Fd != -1 )
			{
				close( pstBlk[k].s32Fd );
				pstBlk[k].s32Fd = -1;
			}
		}
		pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	}
	return;
}

static void fblock_release( finfo_st *pstFileInfo )
{
	fblock_close( pstFileInfo );
	fblock_free( pstFileInfo->pstBlkInfo );
	return;
}

static finfo_st* finfo_alloc( void  )
{
	finfo_st *pstFInfo = NULL;

	pstFInfo = calloc( 1, sizeof( finfo_st ) );
	return ( NULL == pstFInfo ) ? NULL : pstFInfo;
}

static finfo_st* finfo_new( s8 *ps8Url, s8 *ps8FName, u64 u64FSize )
{
	finfo_st *pstFileInfo = NULL;
	
	pstFileInfo = finfo_alloc();
	if( NULL == pstFileInfo )
	{
		return NULL;
	}

	pstFileInfo->ps8FName = ( s8* )strdup( ( char* )ps8FName );
	pstFileInfo->ps8Url= ( s8* )strdup( ( char* )ps8Url );
	pstFileInfo->u32FNameMagic= __string_magic( ps8FName );
	pstFileInfo->u32UrlMagic= __string_magic( ps8Url );
	pstFileInfo->u64Size= u64FSize; 
	pstFileInfo->s32Pause = 0;
	pstFileInfo->u32Upload = 0;
	pthread_mutex_init( &pstFileInfo->mBlkLock, NULL );
	pthread_mutex_init( &pstFileInfo->mInfoLock, NULL );
	return pstFileInfo;
}

static void finfo_release( finfo_st *pstFileInfo )
{

	if( !list_empty(& g_file_head.stHeadList ) )
	{
		if( !list_empty( &pstFileInfo->stList ) )
		{
			list_del_init( &pstFileInfo->stList );
			g_file_head.s32Cnt--;
		}
	}
	
	__finfo_unmap( pstFileInfo );
	if( pstFileInfo->ps8FName != NULL )
	{
		free( pstFileInfo->ps8FName );
		pstFileInfo->ps8FName = NULL;
	}

	if( pstFileInfo->ps8Url != NULL )
	{
		free( pstFileInfo->ps8Url );
		pstFileInfo->ps8Url = NULL;
	}
	
	fblock_release( pstFileInfo );
	pthread_mutex_destroy( &pstFileInfo->mBlkLock );
	pthread_mutex_destroy( &pstFileInfo->mInfoLock );

	finfo_free( pstFileInfo );
	
	return;
}

static finfo_st* __lookup_file_list( s8 *ps8Url, s8 *ps8FName )
{
	u32 u32UrlMagic = 0,
		u32FileMagic = 0;
	finfo_st	*pstFileInfo = NULL;
	
	u32UrlMagic = __string_magic( ps8Url );
	u32FileMagic = __string_magic( ps8FName );
	
	pthread_rwlock_rdlock( &g_file_head.rwLock );
	list_for_each_entry( pstFileInfo, &g_file_head.stHeadList, stList )
	{
		if( u32FileMagic == pstFileInfo->u32FNameMagic &&
			u32UrlMagic == pstFileInfo->u32UrlMagic &&
			0 == strcmp( ( char* )pstFileInfo->ps8FName, ( char* )ps8FName ) &&
			0 == strcmp( ( char* )pstFileInfo->ps8Url, ( char* )ps8Url ) )
		{
			pthread_rwlock_unlock( &g_file_head.rwLock );
			return pstFileInfo;
		}
	}
	pthread_rwlock_unlock( &g_file_head.rwLock );

	return NULL;
}

static s32 __lookup_list( finfo_st *pstFileInfo )
{
	finfo_st	*pstPos = NULL;
	
	pthread_rwlock_wrlock( &g_file_head.rwLock );
	{
		list_for_each_entry( pstPos, &g_file_head.stHeadList, stList )
		{
			if( pstPos == pstFileInfo )
			{
				pthread_rwlock_unlock( &g_file_head.rwLock );
				return 1;
			}
		}
	}
	pthread_rwlock_unlock( &g_file_head.rwLock );
	return 0;
}

static s32 __ftmp_create( finfo_st *pstFileInfo )
{
	s32 s32Fd;
	s32 s32Bytes = 0;
	off_t off = -1;
	ssize_t sSize = -1;
	s8 s8Name[FILE_LEN_MAX] = {0};
	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	s32Fd = open( ( char* )s8Name, O_WRONLY|O_CREAT|O_TRUNC, 0644 );
	if( s32Fd < 0 )
	{
		return -1;
	}

	if( 0 == pstFileInfo->u64Size )
	{
		goto file_start;
	}
	/** set file size */
	off = lseek( s32Fd, pstFileInfo->u64Size - sizeof( s32Bytes ), SEEK_SET );
	if( off < 0 )
	{
		close( s32Fd );
		return -2;
	}
	sSize = write( s32Fd, &s32Bytes, sizeof( s32Bytes ) );
	if( sSize < 0 || sSize != sizeof( s32Bytes ) )
	{
		close( s32Fd );
		return -3;
	}

file_start:	
	/** used by first block */
	off = lseek( s32Fd, 0, SEEK_SET );
	if( off < 0 )
	{
		close( s32Fd );
		return -4;
	}
	pstFileInfo->pstBlkInfo[0].s32Fd = s32Fd;

	return 0;
}

static s32 __fblock_open( fblk_st *pstBlkInfo, s8* ps8FileName, u32 upload )
{
	off_t off = -1;
	mode_t mode = O_WRONLY;
	
	if( pstBlkInfo->s32Fd < 0 )
	{
		s32 s32Fd = -1;
		s8 s8Name[FILE_LEN_MAX] = {0};

		if( !upload )
		{
			mode = O_WRONLY;
			snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, ps8FileName );
		}
		else
		{
			mode = O_RDONLY;
			strncpy( ( char* )s8Name, ps8FileName, sizeof( s8Name ) );
		}
		
		s32Fd = open( ( char* )s8Name, mode );
		if( s32Fd < 0 )
		{
			return -1;
		}

		pstBlkInfo->s32Fd = s32Fd;
		off = lseek( s32Fd, pstBlkInfo->u64Start + pstBlkInfo->u64Offset, SEEK_SET );
		if( off < 0 )
		{
			close( s32Fd );
			return -2;
		}
	}

	return pstBlkInfo->s32Fd;
}


static s32 __finfo_map( finfo_st *pstFileInfo, size_t sSize )
{
 	s32 s32Fd = -1;
 	s32 s32Bytes = 0;

	s8 *ps8Map = NULL;
	size_t sLength = sSize;
	off_t off = -1;
	ssize_t sRet = -1;
 	s8 s8Name[FILE_LEN_MAX] = {0};
	
 	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
 	s32Fd = open( ( char* )s8Name, O_RDWR|O_CREAT, 0644 );
 	if( s32Fd < 0 )
 	{
 		return -1;
 	}

	if( 0 == sLength )
	{/** new file */
		sLength = sizeof( finfo_st ) + 2 * FILE_LEN_MAX + 
				sizeof( fblk_st ) * pstFileInfo->s32BlkCnt;
		
		off = lseek( s32Fd, sLength, SEEK_SET );
		if( off < 0 )
		{
			close( s32Fd );
			return -2;
		}
		sRet = write( s32Fd, &s32Bytes, sizeof( s32Bytes ) );
		if( sRet < 0 || sRet != sizeof( s32Bytes ) )
		{
			close( s32Fd );
			return -3;
		}
	}
	off = lseek( s32Fd, 0, SEEK_SET );
	if( off < 0 )
	{
		close( s32Fd );
		return -4;
	}

 	ps8Map = mmap( NULL, sLength, PROT_READ|PROT_WRITE, MAP_SHARED, s32Fd, 0 );
 	if( MAP_FAILED == ps8Map )
	{
		close( s32Fd );
		return -5;
	}
	close( s32Fd );
	pstFileInfo->ps8BlkMap = ps8Map;

	return 0;
}
static void __finfo_unmap( finfo_st *pstFileInfo )
{
 	s32 s32Bytes = 0;
	u64	tInfoSize;
 	s8 s8Name[FILE_LEN_MAX] = {0};

	if( pstFileInfo->ps8BlkMap != NULL )
	{
		snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
		__is_file_exist( s8Name, &tInfoSize );
	
		munmap( pstFileInfo->ps8BlkMap, tInfoSize - sizeof( s32Bytes ) );
		pstFileInfo->ps8BlkMap = NULL;
	}
}

static s32 __finfo_parse( finfo_st *pstFileInfo, size_t sSize )
{
	s32 k = 0,
		s32BlkCnt = 0;
	size_t sInfoLen = sizeof( finfo_st ),
		   sBlkLen = sizeof( fblk_st );
	
	s8 *ps8BlkMap = pstFileInfo->ps8BlkMap;
	s8 s8Name[FILE_LEN_MAX] = {0};
	
	fblk_st *pstBlk = NULL;
	fblk_st stBlkTmp;
	finfo_st stFileInfo;

	if( sSize < sInfoLen )
	{
		return -1;
	}
	
	stFileInfo = *( finfo_st* )ps8BlkMap;
	if( stFileInfo.u64Size != pstFileInfo->u64Size ||
		stFileInfo.u32FNameMagic != pstFileInfo->u32FNameMagic ||
		stFileInfo.u32UrlMagic != pstFileInfo->u32UrlMagic )
	{
		return -2;
	}
	memset( s8Name, 0, sizeof( s8Name ) );
	memcpy( s8Name, &ps8BlkMap[sInfoLen], FILE_LEN_MAX );
	s8Name[FILE_LEN_MAX - 1] = 0;
	if( strcmp( ( char* )pstFileInfo->ps8FName, ( char* )s8Name ) != 0 )
	{
		return -3;
	}
	
	sInfoLen += FILE_LEN_MAX;
	memset( s8Name, 0, sizeof( s8Name ) );
	memcpy( s8Name, &ps8BlkMap[sInfoLen], FILE_LEN_MAX );
	s8Name[FILE_LEN_MAX - 1] = 0;
	if( strcmp( ( char* )pstFileInfo->ps8Url, ( char* )s8Name ) != 0 )
	{
		return -4;
	}

	pstFileInfo->s32Pause = 0;
	pstFileInfo->u64CurSize = stFileInfo.u64CurSize;
	pstFileInfo->s32BlkCnt = ( 0 == stFileInfo.s32BlkCnt ) ? 1 : stFileInfo.s32BlkCnt;
	pstFileInfo->u64UpSize = pstFileInfo->u64CurSize + FILE_1M;
	pstBlk = fblock_alloc( pstFileInfo->s32BlkCnt );
	if( NULL == pstBlk )
	{
		return -5;
	}
	
	s32BlkCnt = 0;
	sInfoLen += FILE_LEN_MAX;
	for( k = 0; k< pstFileInfo->s32BlkCnt; k++ )
	{
		stBlkTmp = *( fblk_st* )&ps8BlkMap[sInfoLen + k * sBlkLen];
		if( APX_BLK_EMPTY == stBlkTmp.eState ||
			APX_BLK_HOLD == stBlkTmp.eState ||
			APX_BLK_ALLOCED == stBlkTmp.eState  )
		{
			stBlkTmp.eState = APX_BLK_EMPTY;
			pstBlk[s32BlkCnt] = stBlkTmp;
			pstBlk[s32BlkCnt].s32Idx = s32BlkCnt;
			pstBlk[s32BlkCnt].s32Fd = -1;
			s32BlkCnt++;
		}
	}
	pstFileInfo->s32BlkCnt = s32BlkCnt;
	pstFileInfo->pstBlkInfo = pstBlk;

	return 0;
}

/** finfo_st + fileName + url + fblk_st * n */
static void __finfo_update( finfo_st *pstFileInfo, s8 first )
{
	s32 k = 0,
		s32BlkCnt = 0;
	size_t sInfoLen = sizeof( finfo_st ),
		   sBlkLen = sizeof( fblk_st );
	s8 *ps8BlkMap = pstFileInfo->ps8BlkMap;

	if( NULL == ps8BlkMap )
	{
		return;
	}
	
	*( finfo_st* )ps8BlkMap = *pstFileInfo;
	if( first )
	{
		memcpy( &ps8BlkMap[sInfoLen], 
				pstFileInfo->ps8FName, 
				strlen( ( char* )pstFileInfo->ps8FName ) + 1 );
	}
	
	sInfoLen += FILE_LEN_MAX;
	if( first )
	{
		memcpy( &ps8BlkMap[sInfoLen],
				pstFileInfo->ps8Url,
				strlen( ( char* )pstFileInfo->ps8Url ) + 1 );
	}
	
	s32BlkCnt = 0;
	sInfoLen += FILE_LEN_MAX;
	for( k = 0; k < pstFileInfo->s32BlkCnt; k++ )
	{
		if( APX_BLK_EMPTY == pstFileInfo->pstBlkInfo[k].eState ||
			APX_BLK_HOLD == pstFileInfo->pstBlkInfo[k].eState ||
			APX_BLK_ALLOCED == pstFileInfo->pstBlkInfo[k].eState  )
		{
			*( fblk_st* )&ps8BlkMap[sInfoLen + s32BlkCnt * sBlkLen] = pstFileInfo->pstBlkInfo[k];
			s32BlkCnt++;
		}
	}
	( ( finfo_st* )ps8BlkMap )->s32Pause = 0;
	( ( finfo_st* )ps8BlkMap )->s32BlkCnt = s32BlkCnt;
	
	return;
}

static s32 __finfo_create( finfo_st *pstFileInfo )
 {
	s32 s32Ret = 0;

	s32Ret = __finfo_map( pstFileInfo, 0 );
	if( s32Ret != 0 )
	{
		return -1;
	}

	__finfo_update( pstFileInfo, 1 );	
 	return 0;
 }



static void __file_check_fname( finfo_st *pstFileInfo )
{
	s8 *ps8Str = NULL;
	s8 *ps8NewFile = NULL;
	s32 k = 0,
		s32Ret = 0;
	size_t sLen = 0;
	s8 s8Name[FILE_LEN_MAX];

	/** file*/
	sLen = strlen( ( char* )pstFileInfo->ps8FName );
	s32Ret = __is_file_exist( pstFileInfo->ps8FName, NULL );
	s32Ret <<= 1;
	
	/** .tmp file*/
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	s32Ret |= __is_file_exist( s8Name, NULL );
	if( 0 == s32Ret )
	{/** file and <file>.tmp  都不存在*/
		return;
	}	

	/** rename */
	ps8Str = ( s8* )strrchr( ( char* )pstFileInfo->ps8FName, '.' );
	if( ps8Str != NULL &&
		ps8Str != pstFileInfo->ps8FName &&
		( size_t )( ps8Str - pstFileInfo->ps8FName ) != sLen - 1 )
	{
		*ps8Str++ = '\0';
	}

	ps8NewFile = calloc( 1, sLen + 6 );/** (123) */
	if( NULL == ps8NewFile )
	{
		return;
	}

	k = 0;	
	while( k < 100 )
	{
		k++;
		if( ps8Str != NULL )
		{
			snprintf( ( char* )ps8NewFile, sLen + 6 , "%s(%d).%s",
				pstFileInfo->ps8FName, k, ps8Str );
		}
		else
		{
			snprintf( ( char* )ps8NewFile, sLen + 6 , "%s(%d)",
				pstFileInfo->ps8FName, k );

		}
		s32Ret = __is_file_exist( ps8NewFile, NULL );
		s32Ret <<= 1;

		snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, ps8NewFile );
		s32Ret |= __is_file_exist( s8Name, NULL );
		if( 0 == s32Ret )
		{
			break;
		}
	}
	
	free( pstFileInfo->ps8FName );
	pstFileInfo->ps8FName = ps8NewFile;
	pstFileInfo->u32FNameMagic = __string_magic( ps8NewFile );
	return;
}

static s32 __file_create( finfo_st *pstFileInfo )
{
	s32	s32Ret = 0;
	
	__file_check_fname( pstFileInfo );

	/** .tmp file*/
	s32Ret = __ftmp_create( pstFileInfo );
	if( s32Ret != 0 )
	{
		return -1;
	}

	/** .info file*/
	return  __finfo_create( pstFileInfo );
}

static void __finfo_done( finfo_st *pstFileInfo )
{
 	s8 s8Name[FILE_LEN_MAX] = {0};

	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	rename( ( char* )s8Name, ( char* )pstFileInfo->ps8FName );

	__finfo_unmap( pstFileInfo );
	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
	apx_file_delete( s8Name );

	return;
}

static s32 finfo_reload( finfo_st *pstFileInfo )
{
	s32 s32Ret = 0,
 		s32Bytes = 0;
	u64	tTmpSize = 0,
		tInfoSize = 0;
	s8 s8Name[FILE_LEN_MAX];

	/** .tmp file*/
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	s32Ret = __is_file_exist( s8Name, &tTmpSize );
	s32Ret <<= 1;
	
	/** .info file*/
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
	s32Ret |= __is_file_exist( s8Name, &tInfoSize );
	if( s32Ret ^ 0x3
		|| (  pstFileInfo->u64Size != 0
		&& pstFileInfo->u64Size != ( u64 )tTmpSize )
		||  0 == tInfoSize )
	{
		return -1;
	}

	/** map .info file */
	s32Ret = __finfo_map( pstFileInfo, tInfoSize - sizeof( s32Bytes ) );
	if( s32Ret != 0 )
	{
		return -2;
	}

	/** parse .info to info struct */
	s32Ret = __finfo_parse( pstFileInfo, tInfoSize );
	if( s32Ret != 0 )
	{
		__finfo_unmap( pstFileInfo );
		return -3;
	}
	
	return 0;
}

void apx_file_init( void )
{
	g_file_init = 1;
	return;
}

void apx_file_exit( void )
{
	finfo_st	*pstFileInfo = NULL;
	finfo_st	*pstTmp = NULL;

	if( !g_file_init )
		return;
		
	pthread_rwlock_wrlock( &g_file_head.rwLock );
	list_for_each_entry_safe( pstFileInfo, pstTmp, &g_file_head.stHeadList, stList )
	{
		__finfo_update( pstFileInfo, 1 );
		finfo_release( pstFileInfo );
	}
	pthread_rwlock_unlock( &g_file_head.rwLock );
	pthread_rwlock_destroy( &g_file_head.rwLock );
	g_file_init = 0;
	
	return;
}

FILE_T* apx_file_create( s8 *ps8Url, s8 *ps8FName, u64 u64FSize,
						s32 s32BlkCnt, s32 *ps32Err, u32 u32Upload )
{
	s32 s32Ret = 0;
	u64 u64Size = u64FSize;
	u64	tSize = 0;
	finfo_st	*pstFileInfo = NULL;
	apx_file_e eRet = APX_FILE_SUCESS;
	if( ps32Err != NULL )
	{
		*ps32Err = eRet;
	}
	
	if( NULL == ps8Url || NULL == ps8FName )
	{
		eRet =APX_FILE_PARAM_ERR;
		goto END;
	}

	pstFileInfo = __lookup_file_list( ps8Url, ps8FName );
	if( pstFileInfo != NULL )
	{
		eRet =APX_FILE_DOWNLOADING;
		goto END;
	}

	s32Ret = __is_file_exist( ps8FName, &tSize );
	if( 1 != u32Upload )
	{/** download */
		if( s32Ret && ( 0 == u64FSize || ( u64 )tSize == u64FSize ) )
		{
			eRet =APX_FILE_DOWNLOADED;
			goto END;
		}
	}
	else
	{/** upload */
		if( 0 == s32Ret || 0 == tSize )
		{
			eRet = APX_FILE_UNKOWN;
			goto END;
		}
		u64Size = ( u64 )tSize;
	}
	
	/** not found. alloc it */
	pstFileInfo = finfo_new( ps8Url, ps8FName, u64Size );
	if( NULL ==  pstFileInfo )
	{
		eRet =APX_FILE_NO_MEM;
		goto END;
	}

	pstFileInfo->u32Upload = u32Upload;
	if( 1 != pstFileInfo->u32Upload )
	{
		/** 断点续传: 加载未完成的文件信息结构*/
		s32Ret = finfo_reload( pstFileInfo );
		if( 0 == s32Ret )
		{
			eRet =APX_FILE_DOWNLOADING;
			goto ADD_LIST;
		}
	}

	/** new file */
	// divide file to block		 
	pstFileInfo->s32BlkCnt = s32BlkCnt;
	s32Ret = fblock_new( pstFileInfo );
	if( s32Ret != 0 )
	{
		eRet =APX_FILE_NO_MEM;
		goto END;
	}

	if( 1 != pstFileInfo->u32Upload )
	{
		/**  touch .tmp & .info*/
		s32Ret = __file_create( pstFileInfo );
		if( s32Ret != 0 )
		{
			eRet =APX_FILE_UNKOWN;
			goto END;
		}
	}

ADD_LIST:
	pthread_rwlock_wrlock( &g_file_head.rwLock );
	list_add( &pstFileInfo->stList,&g_file_head.stHeadList );
	g_file_head.s32Cnt++;
	pthread_rwlock_unlock( &g_file_head.rwLock );

END:
	if( ps32Err != NULL )
	{
		*ps32Err = eRet;
	}

	if( APX_FILE_DOWNLOADING == eRet ||
		APX_FILE_SUCESS == eRet )
	{
		return pstFileInfo;
	}
	
	if( pstFileInfo != NULL )
	{
		pthread_rwlock_wrlock( &g_file_head.rwLock );
		finfo_release( pstFileInfo );
		pthread_rwlock_unlock( &g_file_head.rwLock );
	}
	
	return NULL;
}

int apx_file_reset( FILE_T *pFileInfo )
{
	s32 k = 0;
	s8 s8Name[FILE_LEN_MAX];
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	fblk_st *pstBlk = pstFileInfo->pstBlkInfo;
	
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->s32Pause = 0;
	pstFileInfo->u64CurSize = 0;
	pstFileInfo->u64UpSize = pstFileInfo->u64CurSize + FILE_1M;
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	
	if( pstBlk != NULL )
	{
		for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
		{
			pstBlk[k].u64Offset = 0;
			pstBlk[k].u8Proxy = 0;
			pstBlk[k].eState = APX_BLK_EMPTY;
			if( pstBlk[k].s32Fd != -1 )
			{
				close( pstBlk[k].s32Fd );
				pstBlk[k].s32Fd = -1;
			}
		}
	}
	
	__finfo_update( pstFileInfo, 1 );
	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	apx_file_delete( s8Name );
	__ftmp_create( pstFileInfo );
	
	return 0;
}

void apx_file_release( FILE_T *pFileInfo )
{	
	//int flag = 0;
	//finfo_st	*pstPos = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo )
	{
		return;
	}

	if( __lookup_list( pstFileInfo ) )
	{
		pthread_rwlock_wrlock( &g_file_head.rwLock );
		finfo_release( pstFileInfo );
		pthread_rwlock_unlock( &g_file_head.rwLock );
	}
	return;
}

void apx_file_delete( s8* pFileName )
{
	if( pFileName != NULL )
	{
		remove( ( char* )pFileName );
	}
	return;
}

void apx_file_delete_all( s8* pFileName )
{
	s32 s32Err = 0;
	
	if( NULL == pFileName )
	{
		return;
	}

	s32Err = apx_file_is_exist( pFileName, NULL );
	if( s32Err )
	{
		apx_file_delete( pFileName );
	}
	else
	{
		s8 s8Name[FILE_LEN_MAX];
		
		snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pFileName );
		s32Err = apx_file_is_exist( s8Name, NULL );
		if( s32Err )
		{
			apx_file_delete( s8Name );
		}
		
		snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pFileName );
		s32Err = apx_file_is_exist( s8Name, NULL );
		if( s32Err )
		{
			apx_file_delete( s8Name );
		}
	}

	return;
}

void apx_file_destroy( FILE_T *pFileInfo)
{
	s8 s8Name[FILE_LEN_MAX];
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo )
	{
		return;
	}
	if( !__lookup_list( pstFileInfo ) )
	{
		return;
	}
	
	if( ( 1 != pstFileInfo->u32Upload ) 
		&& pstFileInfo->ps8FName != NULL )
	{
		__finfo_unmap( pstFileInfo );
		if( pstFileInfo->u64Size == pstFileInfo->u64CurSize )
		{
			apx_file_delete( pstFileInfo->ps8FName );
		}
		else
		{
			snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
			apx_file_delete( s8Name );
			
			snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
			apx_file_delete( s8Name );
		}
	}
	
	apx_file_release( pFileInfo );
	return;
}

void apx_file_pause( FILE_T *pFileInfo )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( pstFileInfo != NULL && 0 == pstFileInfo->s32Pause )
	{
		pstFileInfo->s32Pause = 1;
		fblock_close( pstFileInfo );
		__finfo_update( pstFileInfo, 1 );
	}

	return;
}

void apx_file_resume( FILE_T *pFileInfo )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( pstFileInfo != NULL && 1 == pstFileInfo->s32Pause )
	{
		pstFileInfo->s32Pause = 0;
	}
	return;
}

apx_fblk_st* apx_file_blk_info( FILE_T *pFileInfo )
{
	s32 k = 0;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	fblk_st *pstBlk = pstFileInfo->pstBlkInfo;
	
	if( NULL == pFileInfo || NULL == pstBlk )
	{
		return NULL;
	}
	if( pstFileInfo->s32Pause )
	{
		return NULL;
	}

	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
	{
		if( APX_BLK_EMPTY == pstBlk[k].eState )
		{
			pstBlk[k].s32Idx = k;
			pstBlk[k].eState = APX_BLK_ALLOCED;
			pthread_mutex_unlock( &pstFileInfo->mBlkLock );
			return ( apx_fblk_st* )&pstBlk[k];
		}
	}
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );

	return NULL;
}

void apx_file_blk_reset( FILE_T *pFileInfo, apx_fblk_st *pstblk )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	fblk_st *pstBlkInfo = ( fblk_st* )pstblk;

	if( pstFileInfo != NULL && pstblk != NULL )
	{
		pthread_mutex_lock( &pstFileInfo->mBlkLock );
		pstFileInfo->u64CurSize -= pstBlkInfo->u64Offset;
		pthread_mutex_unlock( &pstFileInfo->mBlkLock );

		pstBlkInfo->u64Offset = 0;
		pstBlkInfo->u8Proxy = 0;
		pstBlkInfo->eState = APX_BLK_EMPTY;
	}
	return;
}

s32 apx_file_blk_cnt( FILE_T *pFileInfo )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	return ( pstFileInfo != NULL ) ? pstFileInfo->s32BlkCnt : 0;
}

ssize_t apx_file_read( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo )
{
	s32 s32Fd = -1;
	u64	u64Left = 0;
	size_t sLen = szSize;
	ssize_t ssRet = -1;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo || s32Idx <0 ||
		s32Idx >= pstFileInfo->s32BlkCnt )
	{
		return -1;
	}
	if( pstFileInfo->s32Pause )
	{
		return -2;
	}

	pstBlk = &( pstFileInfo->pstBlkInfo[s32Idx] );
	if( APX_BLK_FULL == pstBlk->eState )
	{
		return 0;
	}
	if( pstBlk->eState != APX_BLK_ALLOCED  &&
	     pstBlk->eState != APX_BLK_HOLD )
	{
		return -3;
	}
	if( APX_BLK_ALLOCED == pstBlk->eState )
	{
		pstBlk->eState = APX_BLK_HOLD;
	}

	/** left bytes to read*/
	if( pstBlk->u64End > 0 )
	{
		u64Left = pstBlk->u64End -pstBlk->u64Start + 1 -  pstBlk->u64Offset;
		if( u64Left <= szSize )
		{
			sLen = u64Left;
		}
	}
	s32Fd = __fblock_open( pstBlk, pstFileInfo->ps8FName, 1 );
	if( s32Fd < 0 )
	{		
		return -5;
	}
	ssRet = read( s32Fd, pBuf, sLen );
	if( ssRet < 0 || ssRet != ( ssize_t )sLen )
	{
		return -4;
	}

	pstBlk->u64Offset += sLen;
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->u64CurSize += sLen;	
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );

	if( u64Left != 0 && u64Left <= szSize )
	{
		/** full bolck */
		close( pstBlk->s32Fd );
		pstBlk->s32Fd = -1;
		pstBlk->eState = APX_BLK_FULL;
	}
		
	return sLen;
}

ssize_t apx_file_write( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo )
{
	u8	u8Flag = 0;
	s32 s32Fd = -1;
	u64	u64Left = 0;
	size_t sLen = szSize;
	ssize_t ssRet = -1;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo || s32Idx <0 ||
		s32Idx >= pstFileInfo->s32BlkCnt )
	{
		return -1;
	}
	if( pstFileInfo->s32Pause )
	{
		return -2;
	}

	/** file download done */
	if( NULL == pBuf && 0 == szSize )
	{
		pstFileInfo->u64Size = pstFileInfo->u64CurSize;
		fblock_close( pstFileInfo );
		goto file_full;
	}

	if( NULL == pBuf
		|| 0 == szSize )
	{
		return -1;
	}

	pstBlk = &( pstFileInfo->pstBlkInfo[s32Idx] );
	if( pstBlk->eState != APX_BLK_ALLOCED  &&
	     pstBlk->eState != APX_BLK_HOLD )
	{
		return -3;
	}
	if( APX_BLK_ALLOCED == pstBlk->eState )
	{
		pstBlk->eState = APX_BLK_HOLD;
	}
	
	/** left bytes to write*/
	if( pstBlk->u64End > 0 )
	{
		u64Left = pstBlk->u64End -pstBlk->u64Start + 1 -  pstBlk->u64Offset;
		if( u64Left <= szSize )
		{
			sLen = u64Left;
		}
	}
	s32Fd = __fblock_open( pstBlk, pstFileInfo->ps8FName, 0 );
	ssRet = write( s32Fd, pBuf, sLen );
	if( ssRet < 0 || ssRet != ( ssize_t )sLen )
	{
		return -4;
	}	

 	pstBlk->u64Offset += sLen;
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->u64CurSize += sLen;

	if( pstFileInfo->u64CurSize > pstFileInfo->u64UpSize )
	{
		u8Flag = 1;
		pstFileInfo->u64UpSize = pstFileInfo->u64CurSize + FILE_1M;
	}
	if( 1 == pstBlk->u8Proxy )
	{
		pstFileInfo->u64ProxySize += sLen;
	}
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );


	if( u64Left != 0 && u64Left <= szSize )
	{
		/** full bolck */
		close( pstBlk->s32Fd );
		pstBlk->s32Fd = -1;
		pstBlk->eState = APX_BLK_FULL;
		
		if( 0 == pstFileInfo->u32Upload )
		{
			if( pstFileInfo->u64Size == pstFileInfo->u64CurSize )
			{
			file_full:
				__finfo_done( pstFileInfo );
				return sLen;
				//return APX_FILE_FULL;
			}
		}
		__finfo_update( pstFileInfo, 0 );
		return sLen;
		//return APX_FILE_BLK_FULL;
	}
	else if( u8Flag )
	{
		__finfo_update( pstFileInfo, 0 );
	}
	return sLen;
}

u64 apx_file_size( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo )->u64Size : 0;
}

u64 apx_file_cur_size( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo)->u64CurSize : 0;
}

u64 apx_file_proxy_size( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo)->u64ProxySize : 0;
}

void apx_file_set_cur_size( FILE_T *pFileInfo, u64 u64Size )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( pstFileInfo )
	{
		pthread_mutex_lock( &pstFileInfo->mBlkLock );
		pstFileInfo->u64CurSize = u64Size;
		pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	}
	return;
}

s32 apx_file_is_exist( s8 *path, u64 *pSize )
{
	return ( path ) ?  __is_file_exist( path, pSize ) : 0;
}

s32 apx_file_mkdir( s8* path )
{
	s32 ret = 0;
	size_t k = 0,
		   len = 0;
	s8 s8Dir[512] = {0};

	if( NULL == path )
	{
		return -1;
	}

	strncpy( s8Dir,   path, sizeof( s8Dir ) - 1 );
	len = strlen( s8Dir );
	if( s8Dir[len-1] != '/' )
	{
		strncat( s8Dir, "/", sizeof( s8Dir ) - 1 );
	}
	
	len = strlen( s8Dir );
	for( k = 1; k < len; k++ )
	{
		if( s8Dir[k] == '/' )
		{
			s8Dir[k] = 0;
			ret = access( s8Dir, F_OK );
			if( ret < 0 )
			{
				ret = mkdir( s8Dir, 0755 );
				if( ret < 0 && errno != EEXIST )
				{
					return   -1;
				}
			}
			s8Dir[k]   =   '/';
		}
	}
	
	return   0;
}

void apx_file_update_upsize( FILE_T *pFileInfo )
{
	s32 k = 0;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( pstFileInfo != NULL )
	{
		pstBlk = pstFileInfo->pstBlkInfo;
		
		if( pstBlk != NULL )
		{
			for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
			{
				if( APX_BLK_FULL == pstBlk[k].eState )
				{
					pstFileInfo->u64CurSize += pstBlk[k].u64End -pstBlk[k].u64Start + 1;
				}
			}
		}
	}
	
	return;
}

void* apx_block_point( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo)->pstBlkInfo : NULL;
}

void apx_block_reset( FILE_T *pFileInfo )
{
	s32 k = 0;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( pstFileInfo != NULL )
	{
		pstBlk = pstFileInfo->pstBlkInfo;
		
		if( pstBlk != NULL )
		{
			for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
			{
				pstBlk[k].eState = APX_BLK_EMPTY;
				pstBlk[k].u64Offset = 0;
				pstBlk[k].u8Proxy = 0;				
			}
		}
	}
	
	return;
}

void apx_block_set_status( void* pBlk, int index, int status )
{
	fblk_st *pBlkInfo = ( fblk_st* )pBlk;

	if( pBlkInfo != NULL )
	{
		pBlkInfo += index;
		if( pBlkInfo != NULL )
		{
			if( 0 == status )
			{
				pBlkInfo->eState = APX_BLK_EMPTY;
				pBlkInfo->u64Offset = 0;
				pBlkInfo->u8Proxy = 0;				
			}
			else
			{
				pBlkInfo->eState = APX_BLK_FULL;
			}
		}
	}
	return;
}

/*-------------------------------------------------
*Function:    __string_decompress
*Description:   
*      decompressing string
*Parameters: 
*	src[in]             gzip string 
*	dest[OUT]        ungzip string 
*	destlen[OUT]    length of ungzip string
*Return:
*       return 0 if success,other return negative.
*History:
*     guozw     	  2015-9-2        Initial Draft 
*---------------------------------------------------*/
int httpgzdecompress(FILE *source, FILE *dest)
{
	int ret;
	unsigned have;
	z_stream d_stream = {0}; 
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	d_stream.zalloc = (alloc_func)0;
	d_stream.zfree = (free_func)0;
	d_stream.opaque = (voidpf)0;
	d_stream.next_in = Z_NULL;
	d_stream.avail_in = 0;
	d_stream.next_out = Z_NULL;
	
	if(inflateInit2(&d_stream, 47) != Z_OK) 
		return -1;

    /* decompress until deflate stream ends or end of file */
	do{
		d_stream.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) 
		{
			(void)inflateEnd(&d_stream);
			return Z_ERRNO;
		}
		if (d_stream.avail_in == 0)
			break;
		d_stream.next_in = in;

		/* run inflate() on input until output buffer not full */
		do {
			d_stream.avail_out = CHUNK;
			d_stream.next_out = out;
			ret = inflate(&d_stream, Z_NO_FLUSH);
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&d_stream);
					return ret;
			}
			have = CHUNK - d_stream.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) 
			{
				(void)inflateEnd(&d_stream);
				return Z_ERRNO;
			}
		} while (d_stream.avail_out == 0);

        /* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&d_stream);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/*-------------------------------------------------
*Function:    apx_file_ungzip
*Description:   
*      decompressing files
*Parameters: 
*	pFileInfo[in]   file informaction 
*	s32Idx[IN]     block index
*Return:
*       return 0 if success,other return negative.
*History:
*     guozw     	  2015-9-2        Initial Draft 
*---------------------------------------------------*/

s32 apx_file_ungzip( FILE_T *pFileInfo, s32 s32Idx  )
{
	s32 s32Ret = 0;
	s32 rd_len = 0, wt_len = 0;	
	s8 tmp_buf[FILE_1K] = {0};
	u64 total_len = 0;
	s8 s8Name[FILE_LEN_MAX] = {0};
	s8 s8Name_ungzip[FILE_LEN_MAX] = {0};
	//u8 u8Buf[41] = { 0 };
	FILE *pFile = NULL;
	FILE* pFile_unzgip = NULL;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo || s32Idx <0 ||
		s32Idx >= pstFileInfo->s32BlkCnt )
	{
		return -1;
	}
	pstBlk = &( pstFileInfo->pstBlkInfo[s32Idx] );

	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	snprintf( ( char* )s8Name_ungzip, sizeof( s8Name_ungzip ), "%s"FILE_TMP_UNGZIP, pstFileInfo->ps8FName );
	
	pFile = fopen( s8Name, "rb+" );
	if( NULL == pFile )
	{
		return -3;
	}
	pFile_unzgip = fopen( s8Name_ungzip, "wb+");
	if( NULL == pFile_unzgip )
	{
		fclose(pFile);
		return -3;
	}
	
	/** file block ungzip */
	s32Ret= httpgzdecompress(pFile, pFile_unzgip);
	if (s32Ret != Z_OK)
	{
		fclose(pFile);
		fclose(pFile_unzgip);
		return -6;
	}
	fflush(pFile_unzgip);
	fclose(pFile_unzgip);

	pFile_unzgip = fopen( s8Name_ungzip, "rb" );
	fseek( pFile, pstBlk->u64Start, SEEK_SET );
	while( (rd_len = fread(tmp_buf, 1, FILE_1K-1, pFile_unzgip) ) > 0 )
	{
		wt_len = fwrite( tmp_buf, 1, rd_len, pFile);
		total_len += wt_len;
		#if 1
		if (pstFileInfo->u64CurSize <=total_len)
		{
			pthread_mutex_lock( &pstFileInfo->mBlkLock );
			pstFileInfo->u64CurSize =  total_len;
			pthread_mutex_unlock( &pstFileInfo->mBlkLock );
		}
		else
		{
			pthread_mutex_lock( &pstFileInfo->mBlkLock );
			pstFileInfo->u64CurSize += 10;
			pthread_mutex_unlock( &pstFileInfo->mBlkLock );
		}
		#endif
	}
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->u64CurSize =  total_len;
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	pstBlk->u64Offset = (pstBlk->u64End - pstBlk->u64Start + 1);
	fflush( pFile );
	fclose( pFile );
	fclose( pFile_unzgip );

	apx_file_delete( s8Name_ungzip );

	/* set full bolck */
	close( pstBlk->s32Fd );
	pstBlk->s32Fd = -1;
	pstBlk->eState = APX_BLK_FULL;
	__finfo_update( pstFileInfo, 0 );

	if( pstFileInfo->u64Size == pstFileInfo->u64CurSize )
	{
		__finfo_done( pstFileInfo );
	}
	return 0;
}

/*-------------------------------------------------
*Function:    apx_file_is_changed
*Description:   
*   update the origin file name
*Parameters: 
*	pFileInfo[in]    the real fileInfo
*	ps8FName[in]     the origin fileName
*	fname[OUT]       the new fileName for update
*Return:
*   return 0 if success, other return negative.
*History:
*   libg   	  2016-11-3        added
*---------------------------------------------------*/
int apx_file_is_changed(FILE_T *pFileInfo, s8 *ps8FName, s8	*fname) 
{
    if (pFileInfo == NULL || ps8FName == NULL || fname == NULL) 
    {
        return -1;
    }

    finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
    if (pstFileInfo == NULL) 
    {
        return -1;
    }

    if (0 == strcmp ((char*)pstFileInfo->ps8FName, (char*)ps8FName)) 
    {
        return -2;
    }

    char* p_whole_name = rindex(pstFileInfo->ps8FName, '/');
    if (p_whole_name == NULL)
    {
        return -3;
    }

    p_whole_name++;
    strcpy((char*)fname, p_whole_name);
    return 0;
}

