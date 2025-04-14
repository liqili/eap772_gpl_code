/*  Copyright(c) 2009-2019 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		tp_auth.h
 * brief		
 * details	
 *
 * author	Han Renjie
 * version	
 * date		29Sep19
 *
 * history 	\arg	
 */

#ifndef __TP_AUTH_H__
#define __TP_AUTH_H__
/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

#ifndef FALSE
#define FALSE		0
#endif
#ifndef TRUE
#define TRUE		1
#endif

#ifndef AUTH_OK
#define AUTH_OK                         (0)
#endif
#ifndef AUTH_ERR
#define AUTH_ERR                        (-1)
#endif

#ifndef AUTH_STATUS
#define AUTH_STATUS                     int
#endif

#ifndef IP_STR_LEN
#define IP_STR_LEN	16
#endif

/* 与ipc_pub.h中定义保持一致 */
#define TP_SSH_AUTH_SHM_KEY_ID			(0x54505353)
#define TP_SSH_AUTH_SEM_KEY_ID			(0x54505354)
#define LOCK_EINTR_RETRY_COUNT			(30)

/* ssh锁定相关警告信息的最大长度 */
#define TP_TIPS_MAX_LEN					(256)

/* 允许密码错误的最大次数 */
#define MAX_AUTH_ERROR_CNT		(30)
/* 考虑认证错误的时间长度：30mins */
#define MAX_AUTH_ERROR_REC_TIME		(1800)
/* 认证错误次数超限后禁止登陆的时间：2hours */
#define MAX_AUTH_ERROR_FORBID_TIME	(7200)
/* 缓存的登陆异常客户端的最大数目 */
#define MAX_AUTH_ERROR_RECORD_NUM	(128)

#define AUTH_ERROR(info,args...) \
	do {\
		printf("<auth>[error]%s(): %d  -> "info"\r\n",__FUNCTION__,__LINE__,##args);\
	} while(0)
	
#define AUTH_DEBUG(info,args...) \
	do {\
		/* printf("<auth>[debug]%s(): %d  -> "info"\r\n",__FUNCTION__,__LINE__,##args); */\
	} while(0)


/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/
#ifndef INT8
typedef char			INT8;
#endif
#ifndef INT16
typedef short			INT16;
#endif
#ifndef INT32
typedef int				INT32;
#endif
#ifndef UINT8
typedef unsigned char	UINT8;
#endif
#ifndef UINT16
typedef unsigned short	UINT16;
#endif
#ifndef UINT32
typedef unsigned int	UINT32;
#endif
#ifndef BOOL
typedef int				BOOL;
#endif

typedef enum
{
	AUTH_SUCCESS,
	AUTH_FAILED
} AUTH_STATE;

typedef enum 
{
	WEB_AUTH_LOCK_UNSET = 0,
	WEB_AUTH_LOCK_NEEDSET,
	WEB_AUTH_LOCK_SETTED
} AUTH_LOCK_TYPE;

#if 0
/* 使用IP区分多个客户端的登录记录 */
typedef struct
{
	UINT8 ip[IP_STR_LEN];
	AUTH_LOCK_TYPE isLocked;
	UINT32 lastLoginTime;	/* 最近一次登录时系统的运行时间 */
	UINT32 lockTime;		/* 用户被锁定时系统的运行时间 */
	UINT8 errCnt;
	UINT32 errRecTime[MAX_AUTH_ERROR_CNT];	/* 每次登录失败时对应的系统运行时间 */
} AUTH_ENTRY;

typedef struct
{
	UINT32 cnt;
	AUTH_ENTRY authEntry[MAX_AUTH_ERROR_RECORD_NUM];
}AUTH_DATA;
#else
typedef struct
{
	AUTH_LOCK_TYPE isLocked;
	UINT32 lockTime;		/* 用户被锁定时系统的运行时间 */
	UINT8 errCnt;
	UINT32 errRecTime[MAX_AUTH_ERROR_CNT];	/* 每次登录失败时对应的系统运行时间 */
} AUTH_DATA;
#endif

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      FUNCTIONS                                                 */
/**************************************************************************************************/

int authGetRetryChances();
int authGetWaitTime();
BOOL authGetLockStatus();
void authUpdateTableWithState(AUTH_STATE authState);
AUTH_STATUS tpAuthInit();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TP_AUTH_H__ */
