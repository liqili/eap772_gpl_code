/*  Copyright(c) 2009-2018 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		apiClientTable.c
 * brief		
 * details	
 *
 * author	Han Renjie
 * version	
 * date		26Jul18
 *
 * history 	\arg	
 */
/**************************************************************************************************/
/*                                           CONFIGURATIONS                                       */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           INCLUDE_FILES                                        */
/**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <linux/if.h>

#include <wireless.h>
#include <wlan_pub.h>
#include <type_pub.h>

#include <ieee80211_external_config.h>
#include <ieee80211_external.h>

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* the same with macros in clientTable_data_access.h */
#define	MAC_ADDR_STRING_LEN   18

#define	DEBUG_CLIENT(fmt,args...)  //printf(fmt, ##args)

/* struct ieee80211req_sta_info + ielen(wpaie+wmeie+athie+wpsie), normally ielen won't > 100 */
#define STA_BUF_LEN 		((sizeof(struct ieee80211req_sta_info) + 100) * MAX_STA_PER_RADIO)

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

typedef struct
{
	UINT8 ifname[IFNAMELEN];
	UINT16 apIdx;
} VAP_IF_ENTRY;

typedef struct
{
	VAP_IF_ENTRY ifentry[MAX_VAP_NUM];
	UINT32 count;
} VAP_IF_LIST;

typedef struct
{
	char macaddr[MAX_STA_NUM][MAC_ADDR_STRING_LEN];
	int clientCnt;
} SNMP_CLIENT_TABLE_DATA;

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

static int l_sock = -1;
static UINT8 l_buf[STA_BUF_LEN] = {0};

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

static const char *
l_ieee80211_ntoa(const u_int8_t mac[6], int bColon)
{
	static char a[18];
	int i;

	if(bColon)
	{
		i = snprintf(a, sizeof(a), "%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	else
	{
		i = snprintf(a, sizeof(a), "%02X-%02X-%02X-%02X-%02X-%02X",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	return (i < 17 ? NULL : a);
}

int _initSock(void)
{
	if (l_sock < 0)
	{
		l_sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (l_sock < 0)
		{
			DEBUG_CLIENT("socket failed, reason: %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

void api_wlan_getVapName(UINT32 apid, char *pBuf)
{
	if (NULL == pBuf)
	{
		return;
	}

	snprintf(pBuf, VAP_NAME_LEN, "ath%d", apid);
}

int api_wlan_getAllVapIflist(VAP_IF_LIST *pList)
{
	int i = 0;
	int j = 0;
	unsigned int apIdx = 0;
	int num = 0;
	char ifname[VAP_NAME_LEN] = {0};
	struct ifreq ifr;

	if (NULL == pList)
	{
		DEBUG_CLIENT("invalid param\n");
		return -1;
	}

	if (0 != _initSock())
	{
		return -1;
	}

	num = 0;
	for (i = 0; i < MAX_RADIO_NUM; i++)
	{
		for (j = 0; j < MAX_VAP_PER_RADIO; j++)
		{
			apIdx = i * 10 + j;
			api_wlan_getVapName(apIdx, ifname);

			/* check if vap exist and is up state */
			memset(&ifr, 0, sizeof(ifr));
			strncpy(ifr.ifr_name, ifname, IFNAMELEN);
			if (ioctl(l_sock, SIOCGIFFLAGS, &ifr) < 0)
			{
				DEBUG_CLIENT("if:%s not up/exist, reason:%s\n", ifname, strerror(errno));
				continue;
			}
			if(ifr.ifr_flags & IFF_RUNNING) 
			{
				pList->ifentry[num].apIdx = apIdx;
				memcpy(pList->ifentry[num].ifname, ifname, IFNAMELEN);
				num++;
			}
		}
	}

	pList->count = num;

	return 0;
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

void api_wlanGetClientInfo(SNMP_CLIENT_TABLE_DATA *pData)
{
	struct iwreq *piwr = NULL;
	struct ifreq ifr;
	UINT8* pPos = NULL;
	UINT8 *pExtBuf = NULL;
	int reqSpace = 0;
	struct ieee80211req_sta_info *pSi = NULL;
	int len = 0;
	int i = 0, n = 0;
	long int uptime = 0;
	VAP_IF_LIST iflist;

	if (NULL == pData)
	{
		return ;
	}

	if (0 != _initSock())
	{
		return ;
	}

	memset(&l_buf, 0, sizeof(l_buf));
	memset(&iflist, 0, sizeof(iflist));
	/* get sta from each vap */
	api_wlan_getAllVapIflist(&iflist);
	if (0 == iflist.count)
	{
		DEBUG_CLIENT("none of the vap is up \n");
		return ;
	}

	for (i = 0; i < iflist.count; i++)
	{
		memset(&ifr, 0, sizeof(ifr));
		piwr = (struct iwreq *)&ifr;
		strncpy(piwr->ifr_ifrn.ifrn_name, iflist.ifentry[i].ifname, sizeof(piwr->ifr_ifrn.ifrn_name));
		piwr->u.data.pointer = (void *) l_buf;

		len = sizeof(l_buf);
		piwr->u.data.flags = GET_IWREQ_IOCTL_FLAGS(len);
		piwr->u.data.length = GET_IWREQ_IOCTL_REQLEN(len);

		reqSpace = ioctl(l_sock, IEEE80211_IOCTL_STA_INFO, piwr);
		if (reqSpace < 0)
		{
			DEBUG_CLIENT("ioctl failed, reason: %s\n", strerror(errno));
			continue;
		}
		else if (reqSpace > 0)
		{
			/* XXX: malloc memory */
			if (NULL == (pExtBuf = malloc(reqSpace)))
			{
				DEBUG_CLIENT("malloc failed, reason: %s\n", strerror(errno));
				continue;
			}
			piwr->u.data.pointer = (void *) pExtBuf;
			piwr->u.data.flags = GET_IWREQ_IOCTL_FLAGS(reqSpace);
			piwr->u.data.length = GET_IWREQ_IOCTL_REQLEN(reqSpace);
			if (ioctl(l_sock, IEEE80211_IOCTL_STA_INFO, piwr) < 0)
			{
				DEBUG_CLIENT("ioctl failed, reason: %d\n", errno);
				SAFE_FREE(pExtBuf);
				continue;
			}
			pPos = pExtBuf;
		}
		else
		{
			pPos = l_buf;
		}

    	len = GET_IWREQ_IOCTL_RETLEN(piwr->u.data.flags, piwr->u.data.length);
		if (len < sizeof(struct ieee80211req_sta_info))
		{
			DEBUG_CLIENT("vap:%s, sta info too short\n", iflist.ifentry[i].ifname);
			SAFE_FREE(pExtBuf);
			continue;
		}

		while (len >= sizeof(struct ieee80211req_sta_info) && n < MAX_STA_NUM)
		{
			pSi = (struct ieee80211req_sta_info *) pPos;

			memcpy((char*)pData->macaddr[n], l_ieee80211_ntoa(pSi->isi_macaddr, 0), MAC_ADDR_STRING_LEN);
			DEBUG_CLIENT("sta: %s\n", pData->macaddr[n]);
			DEBUG_CLIENT("num: %x:%x:%x\n", pSi->isi_macaddr[0], pSi->isi_macaddr[1], pSi->isi_macaddr[2]);
			pPos += pSi->isi_len, len -= pSi->isi_len;

			n ++;
		}

		SAFE_FREE(pExtBuf);
	}
	pData->clientCnt= n;

	return ;
}

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/



