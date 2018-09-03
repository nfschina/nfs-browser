#ifndef  WLOG_H
#define  WLOG_H

#include <semaphore.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"{
#endif

#define NORMAL_LOG 0
#define DEBUG_LOG
#define BUF_LEN 2048

/********************************
	@brief  	
		release the log module,clean log_mutex and close log_fp

	@param[in] 
		none
		
	@return
		none
*********************************/
void apx_wlog_release();

/********************************
	@brief  	
		init the log module
		
	@param[in] file
		file pointer to log file
		
	@return
		success:   0
		failed:     -1
*********************************/
int apx_wlog_init(const char *file);

/********************************
	@brief  	
		write the log
		
	@param[in] szlog
		pointer to the log msg which want to write in the log file
	@param[in] errcode
		errno or use defined error code,if errcode=NORMAL_LOG, no errcode output
		
	@return
		success:   0
		failed:     -1
*********************************/
int apx_wlog(const char* szlog,int errcode);

/********************************
	@brief  	
		write the log and exit the program
		
	@param[in] szlog
		pointer to the log msg which want to write in the log file
	@param[in] errcode
		errno or use defined error code,if errcode=0,no errcode output
		
	@return
		none
*********************************/
void apx_wlog_quit(const char* szlog,int errcode); 


/********************************
	@brief  	
		write the log with variable-length parameter
		
	@param[in] fmt
		format string
		
	@return
		success: 0
    failed:  -1	
*********************************/
int apx_vlog(const char* fmt, ...);

#ifdef __cplusplus
};
#endif

#endif
