/*  Copyright(c) 2009-2019 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		tp_auth.c
 * brief		记录登陆过SSH的客户端信息，用于防止恶意登录
 * details	
 *
 * author	Han Renjie
 * version	
 * date		29Sep19
 *
 * history 	\arg	
 */

/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include "tp_auth.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

static int l_shmId = -1;
static int l_semId = -1;
static int l_authInited = 0;

/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

static int _sem_lock(int semId)
{
    int ret = -1;
    int i   = 0;
    struct sembuf semBuf;

    semBuf.sem_num = 0;
    semBuf.sem_op = -1;
    semBuf.sem_flg = SEM_UNDO;

	if (semId == -1)
	{
		return -1;
	}

    for (i = 0; i < 30 && ret; i++)
    {
        ret = semop(semId, &semBuf, 1);
        if (ret != 0)
        {
            if (errno == EINTR || ret == EINTR)
            {
                AUTH_ERROR("lock interrupted by signal");
                ret = -1;
            }
            else if (errno == EAGAIN || ret == EAGAIN)
            {
                AUTH_ERROR("timed out, errno=%d rc=%d", errno, ret);
                ret =  -1;
                break;
            }
            else
            {
                AUTH_ERROR("lock failed, errno=%d rc=%d", errno, ret);
                ret = -1;
                break;
            }
        }
        else
        {
            AUTH_DEBUG("pid %d lock succeed.", getpid());
        }
    }

    return ret;
}

static int _sem_unlock(int semId)
{
    struct sembuf semBuf;
    int ret = 0;

    semBuf.sem_num = 0;
    semBuf.sem_op = 1;
    semBuf.sem_flg = SEM_UNDO;

	if (-1 == semId)
	{
		return -1;
	}

    if (semop(semId, &semBuf, 1) == -1)
    {
        AUTH_ERROR("release lock failed, errno=%d", errno);
        ret = -1;
    }
    else
    {
        AUTH_DEBUG("pid %d unlock succeed.", getpid());
    }

    return ret;
}

static int _auth_init()
{
	if (1 == l_authInited)
	{
		return 0;
	}

	/* initial shared men */
	if ((l_shmId = shmget(TP_SSH_AUTH_SHM_KEY_ID, 0, 0)) < 0)
	{
		AUTH_ERROR("shmget failed, reason: %d", errno);
		return -1;
	}

	/* initial sem */
	if ((l_semId = semget(TP_SSH_AUTH_SEM_KEY_ID, 0, 0)) == -1)
	{
		AUTH_ERROR("semget failed, reason: %d", errno);
		return -1;
	}

	l_authInited = 1;
	return 0;
}

#if 0
static int _authGetEntryIdxWithIp(UINT8 *pIp, AUTH_DATA *pAuthData)
{
	int i = 0;

	if (NULL == pIp || 0 == strlen(pIp) || NULL == pAuthData)
	{
		return -1;
	}

	for (i = 0; i < MAX_AUTH_ERROR_RECORD_NUM; i++)
	{
		if (0 != strlen(pAuthData->authEntry[i].ip)
			&& 0 == memcmp(pAuthData->authEntry[i].ip, pIp, IP_STR_LEN))
		{
			break;
		}
	}

	if (MAX_AUTH_ERROR_RECORD_NUM == i)
	{
		/* not found */
		return -1;
	}

	return i;
}

static void _authAddEntry(UINT8 *pIp, unsigned long uptime, AUTH_DATA *pAuthData)
{
	int i = 0;

	if (NULL == pIp || 0 == strlen(pIp) || NULL == pAuthData)
	{
		return ;
	}

	for (i = 0; i < MAX_AUTH_ERROR_RECORD_NUM; i++)
	{
		if (0 == strlen(pAuthData->authEntry[i].ip))
		{
			memset(&(pAuthData->authEntry[i]), 0, sizeof(AUTH_ENTRY));
			memcpy(pAuthData->authEntry[i].ip, pIp, IP_STR_LEN);
			pAuthData->authEntry[i].lastLoginTime = uptime;
			pAuthData->authEntry[i].errRecTime[0] = uptime;
			pAuthData->authEntry[i].errCnt = 1;
			pAuthData->cnt++;
			break;
		}
	}
}

static void _authDelEntry(UINT8 *pIp, AUTH_DATA *pAuthData)
{
	int i = 0;

	if (NULL == pIp || 0 == strlen(pIp) || NULL == pAuthData)
	{
		return ;
	}

	for (i = 0; i < MAX_AUTH_ERROR_RECORD_NUM; i++)
	{
		if (0 != strlen(pAuthData->authEntry[i].ip)
			&& 0 == memcmp(pAuthData->authEntry[i].ip, pIp, IP_STR_LEN))
		{
			memset(&(pAuthData->authEntry[i]), 0, sizeof(AUTH_ENTRY));
			pAuthData->cnt--;
			break;
		}
	}
}

static void _authDelOldestEntry(AUTH_DATA *pAuthData)
{
	struct sysinfo info;
	int waitTime = 0;
	int tmpTime = 0;
	int i = 0;
	int idx = MAX_AUTH_ERROR_RECORD_NUM;

	if (NULL == pAuthData)
	{
		return ;
	}

	if (0 == pAuthData->cnt)
	{
		return;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	for (i = 0; i < MAX_AUTH_ERROR_RECORD_NUM; i++)
	{
		/* record */
		if (0 != strlen(pAuthData->authEntry[i].ip))
		{
			tmpTime = info.uptime - pAuthData->authEntry[i].lastLoginTime;
			if (tmpTime > 0
				&& waitTime < tmpTime)
			{
				/* 获取上次登录最旧的条目 */
				waitTime = tmpTime;
				idx = i;
			}
		}
	}

	if (idx >= 0 && idx < MAX_AUTH_ERROR_RECORD_NUM)
	{
		memset(&(pAuthData->authEntry[idx]), 0, sizeof(AUTH_ENTRY));
		pAuthData->cnt--;
	}
}

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/

/* 
 * fn		int authGetRetryChances(UINT8 *pIp)
 * brief	
 * details	获取pIp对应客户端的登录重试次数
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
int authGetRetryChances(UINT8 *pIp)
{
	int idx = 0;
	int chances = MAX_AUTH_ERROR_CNT;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (NULL == pIp || 0 == strlen(pIp))
	{
		AUTH_ERROR("pIp is invalid.");
		return MAX_AUTH_ERROR_CNT;
	}

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return MAX_AUTH_ERROR_CNT;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return MAX_AUTH_ERROR_CNT;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return MAX_AUTH_ERROR_CNT;
	}

	pAuthData = (AUTH_DATA *)pShBuf;
	idx = _authGetEntryIdxWithIp(pIp, pAuthData);
	if (-1 != idx)
	{
		chances = MAX_AUTH_ERROR_CNT - pAuthData->authEntry[idx].errCnt;
	}
	_sem_unlock(l_semId);
	shmdt(pShBuf);

	return chances;
}

/* 
 * fn		int authGetWaitTime(UINT8 *pIp)
 * brief	
 * details	获取锁定期账户仍需等待的时间
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
int authGetWaitTime(UINT8 *pIp)
{
	int waitTime = 0;
	struct sysinfo info;
	UINT8 *pShBuf = NULL;
	int idx = 0;
	AUTH_DATA *pAuthData = NULL;

	if (NULL == pIp || 0 == strlen(pIp))
	{
		AUTH_ERROR("pIp is invalid.");
		return 0;
	}

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return 0;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return 0;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return 0;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	pAuthData = (AUTH_DATA *)pShBuf;
	idx = _authGetEntryIdxWithIp(pIp, pAuthData);
	if (-1 != idx)
	{
		waitTime = info.uptime - pAuthData->authEntry[idx].lockTime;
		waitTime = MAX_AUTH_ERROR_FORBID_TIME - waitTime;
	}
	_sem_unlock(l_semId);
	shmdt(pShBuf);

	return waitTime;
}

/* 
 * fn		BOOL authGetLockStatus(UINT8 *pIp)
 * brief	
 * details	获取pIp对应客户端的锁定状态
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
BOOL authGetLockStatus(UINT8 *pIp)
{
	struct sysinfo info;
	int i = 0;
	BOOL ret = FALSE;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (NULL == pIp || 0 == strlen(pIp))
	{
		AUTH_ERROR("pIp is invalid.");
		return FALSE;
	}

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return FALSE;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return FALSE;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return FALSE;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	pAuthData = (AUTH_DATA *)pShBuf;
	if (NULL != pAuthData)
	{
		for (i = 0; i < MAX_AUTH_ERROR_RECORD_NUM; i++)
		{
			if (0 != strlen(pAuthData->authEntry[i].ip) 
				&& 0 == memcmp(pAuthData->authEntry[i].ip, pIp, IP_STR_LEN))
			{
				if (WEB_AUTH_LOCK_SETTED == pAuthData->authEntry[i].isLocked)
				{
					/* check if lock is timeout(2 hours since first locked) */
					if (info.uptime - pAuthData->authEntry[i].lockTime >= MAX_AUTH_ERROR_FORBID_TIME)
					{
						memset(&(pAuthData->authEntry[i]), 0, sizeof(AUTH_ENTRY));
						pAuthData->cnt--;
						ret = FALSE;
					}
					else
					{
						ret = TRUE;
					}
				}
				else
				{
					if (WEB_AUTH_LOCK_NEEDSET == pAuthData->authEntry[i].isLocked)
					{
						/* 在第30次失败时显示的字段与锁定期间不同，所以此处增加一个状态用于过渡 */
						pAuthData->authEntry[i].isLocked = WEB_AUTH_LOCK_SETTED;
					}
					ret = FALSE;
				}
				break;
			}
		}
		if (i == MAX_AUTH_ERROR_RECORD_NUM)
		{
			ret = FALSE;
		}
	}

	_sem_unlock(l_semId);
	shmdt(pShBuf);

	return ret;
}

/* 
 * fn		void authUpdateTableWithState(UINT8 *pIp, AUTH_STATE authState)
 * brief	
 * details	更新pIp对应客户端的条目，如果认证失败，则记录本次失败的时间并更新锁定状态
 *			如果认证成功，则清除之前的失败记录
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void authUpdateTableWithState(UINT8 *pIp, AUTH_STATE authState)
{
	struct sysinfo info;
	int i = 0;
	int j = 0;
	int needSet = 1;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (NULL == pIp || 0 == strlen(pIp))
	{
		AUTH_ERROR("pIp is invalid.");
		return ;
	}

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return ;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return ;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return ;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	pAuthData = (AUTH_DATA *)pShBuf;
	if (NULL != pAuthData)
	{
		if (AUTH_SUCCESS == authState)
		{
			_authDelEntry(pIp, pAuthData);
			goto leave;
		}

		i = _authGetEntryIdxWithIp(pIp, pAuthData);
		if (-1 == i)
		{
			/* add new entry */
			if (pAuthData->cnt == MAX_AUTH_ERROR_RECORD_NUM)
			{
				/* 条目达到限制数目时，删除最旧的条目 */
				_authDelOldestEntry(pAuthData);
			}

			/* 添加新条目并记录第一次错误记录的时间 */
			_authAddEntry(pIp, info.uptime, pAuthData);
			goto leave;
		}

		if (WEB_AUTH_LOCK_UNSET == pAuthData->authEntry[i].isLocked)
		{
			/* clear the count before update record */
			pAuthData->authEntry[i].errCnt = 0;
			for (j = 0; j < MAX_AUTH_ERROR_CNT; j++)
			{
				/* 出错的时间已经不在考虑范围 */
				if (info.uptime - pAuthData->authEntry[i].errRecTime[j] > MAX_AUTH_ERROR_REC_TIME)
				{
					pAuthData->authEntry[i].errRecTime[j] = 0;
				}
				if (0 == pAuthData->authEntry[i].errRecTime[j])
				{
					if (1 == needSet)
					{
						needSet = 0;
						pAuthData->authEntry[i].errRecTime[j] = info.uptime;
						pAuthData->authEntry[i].errCnt++;
					}
				}
				else
				{
					pAuthData->authEntry[i].errCnt++;
				}
			}
		}

		pAuthData->authEntry[i].lastLoginTime = info.uptime;

		if (WEB_AUTH_LOCK_UNSET == pAuthData->authEntry[i].isLocked
			&& MAX_AUTH_ERROR_CNT == pAuthData->authEntry[i].errCnt)
		{
			pAuthData->authEntry[i].isLocked = WEB_AUTH_LOCK_NEEDSET;
			pAuthData->authEntry[i].lockTime = info.uptime;
		}
	}

leave:
	_sem_unlock(l_semId);
	/* disconnect from share memory */
	shmdt(pShBuf);
}
#else
/* 
 * fn		int authGetRetryChances()
 * brief	
 * details	获取剩余登录重试次数
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		处理流程上调用本函数前都会调用authUpdateTableWithState，因此此处就不再判断各次登录的时间
 */
int authGetRetryChances()
{
	int chances = MAX_AUTH_ERROR_CNT;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return MAX_AUTH_ERROR_CNT;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return MAX_AUTH_ERROR_CNT;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return MAX_AUTH_ERROR_CNT;
	}

	pAuthData = (AUTH_DATA *)pShBuf;
	if (NULL != pAuthData)
	{
		chances = MAX_AUTH_ERROR_CNT - pAuthData->errCnt;
	}

	_sem_unlock(l_semId);
	shmdt(pShBuf);

	return chances;
}

/* 
 * fn		int authGetWaitTime()
 * brief	
 * details	获取锁定期仍需等待的时间
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
int authGetWaitTime()
{
	int waitTime = 0;
	struct sysinfo info;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return 0;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return 0;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return 0;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	pAuthData = (AUTH_DATA *)pShBuf;
	if (NULL != pAuthData)
	{
		waitTime = info.uptime - pAuthData->lockTime;
		waitTime = MAX_AUTH_ERROR_FORBID_TIME - waitTime;
	}
	_sem_unlock(l_semId);
	shmdt(pShBuf);

	return waitTime;
}

/* 
 * fn		BOOL authGetLockStatus()
 * brief	
 * details	获取锁定状态
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
BOOL authGetLockStatus()
{
	struct sysinfo info;
	BOOL ret = FALSE;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return FALSE;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return FALSE;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return FALSE;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	pAuthData = (AUTH_DATA *)pShBuf;
	if (NULL != pAuthData)
	{
		if (WEB_AUTH_LOCK_SETTED == pAuthData->isLocked)
		{
			/* check if lock is timeout(2 hours since first locked) */
			if (info.uptime - pAuthData->lockTime >= MAX_AUTH_ERROR_FORBID_TIME)
			{
				memset(pAuthData, 0, sizeof(AUTH_DATA));
				ret = FALSE;
			}
			else
			{
				ret = TRUE;
			}
		}
		else
		{
			if (WEB_AUTH_LOCK_NEEDSET == pAuthData->isLocked)
			{
				/* 第30次失败时显示的字段与锁定期间不同，所以此处返回的锁定状态为FALSE */
				pAuthData->isLocked = WEB_AUTH_LOCK_SETTED;
			}
			ret = FALSE;
		}
	}
	_sem_unlock(l_semId);
	shmdt(pShBuf);

	return ret;
}

/* 
 * fn		void authUpdateTableWithState(AUTH_STATE authState)
 * brief	
 * details	更新认证记录，如果认证失败，则记录本次失败的时间并更新锁定状态
 *			如果认证成功，则清除之前的失败记录
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void authUpdateTableWithState(AUTH_STATE authState)
{
	struct sysinfo info;
	int i = 0;
	int needSet = 1;
	UINT8 *pShBuf = NULL;
	AUTH_DATA *pAuthData = NULL;

	if (0 != _auth_init())
	{
		AUTH_ERROR("_auth_init failed.");
		return ;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(l_shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return ;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(l_semId))
	{
		shmdt(pShBuf);
		return ;
	}

	memset(&info, 0, sizeof(info));
	sysinfo(&info);

	pAuthData = (AUTH_DATA *)pShBuf;
	if (NULL != pAuthData)
	{
		if (AUTH_SUCCESS == authState)
		{
			memset(pAuthData, 0, sizeof(AUTH_DATA));
			goto leave;
		}

		if (WEB_AUTH_LOCK_UNSET == pAuthData->isLocked)
		{
			/* clear the count before update record */
			pAuthData->errCnt = 0;
			for (i = 0; i < MAX_AUTH_ERROR_CNT; i++)
			{
				/* 出错的时间已经不在考虑范围 */
				if (info.uptime - pAuthData->errRecTime[i] > MAX_AUTH_ERROR_REC_TIME)
				{
					pAuthData->errRecTime[i] = 0;
				}
				if (0 == pAuthData->errRecTime[i])
				{
					if (1 == needSet)
					{
						needSet = 0;
						pAuthData->errRecTime[i] = info.uptime;
						pAuthData->errCnt++;
					}
				}
				else
				{
					pAuthData->errCnt++;
				}
			}
		}

		if (WEB_AUTH_LOCK_UNSET == pAuthData->isLocked
			&& MAX_AUTH_ERROR_CNT == pAuthData->errCnt)
		{
			pAuthData->isLocked = WEB_AUTH_LOCK_NEEDSET;
			pAuthData->lockTime = info.uptime;
		}
	}

leave:
	_sem_unlock(l_semId);
	/* disconnect from share memory */
	shmdt(pShBuf);
}
#endif
/* 
 * fn		AUTH_STATUS tpAuthInit()
 * brief	
 * details	初始化共享内存用于保存登录失败的客户端信息
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		由于dropbear处理客户端的SSH连接是通过fork创建子进程实现的，因此此处必须使用共享内存
 *			与信号量来同步各子进程的数据
 */
AUTH_STATUS tpAuthInit()
{
	int shmId = -1;
	int semId = -1;
	UINT8 *pShBuf = NULL;
	AUTH_DATA authData;

	AUTH_DEBUG("authInit called");

	/* initial shm */
	if ((shmId = shmget(TP_SSH_AUTH_SHM_KEY_ID, sizeof(AUTH_DATA), IPC_CREAT | 0666)) < 0)
	{
		AUTH_ERROR("shmget failed, reason: %s", strerror(errno));
		return -1;
	}

	/* initial sem */
	semId = semget(TP_SSH_AUTH_SEM_KEY_ID, 1, IPC_CREAT | 0666);
	if (semId == -1)
	{
		AUTH_ERROR("semget failed, reason: %s", strerror(errno));
		return AUTH_ERR;
	}
	/*
	 * We are creating new semaphore, so initialize semaphore to 1
	 */
	if(semctl(semId, 0, SETVAL, 1) == -1)
	{
		AUTH_ERROR("setctl setval 1 failed, reason: %s", strerror(errno));
		if (semctl(semId, 0, IPC_RMID) < 0)
		{
			AUTH_ERROR("semctl IPC_RMID failed, reason: %s", strerror(errno));
		}
		return AUTH_ERR;
	}

	/* connect to share mem */
	if (NULL == (pShBuf = shmat(shmId, NULL, 0)))
	{
		AUTH_ERROR("failed to connect share mem, reason: %s", strerror(errno));
		return AUTH_ERR;
	}
	
	/* copy to share mem */
	if (-1 == _sem_lock(semId))
	{
		shmdt(pShBuf);
		return AUTH_ERR;
	}

	/* 每次SSH初次启动时都会重新初始化共享内存区，这样用户关闭再开启ssh后就不会残留之前的记录 */
	memset(&authData, 0, sizeof(AUTH_DATA));
	memcpy(pShBuf, &authData, sizeof(authData));

	_sem_unlock(semId);
	/* disconnect from share memory */
	shmdt(pShBuf);

	return AUTH_OK;
}

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/



