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

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* the same with macros in clientTable_data_access.h */
#define	MAC_ADDR_STRING_LEN   18
#define	MAX_CLIENT_NUM	100

#define	DEBUG_CLIENT(fmt,args...)  //printf(fmt, ##args)

/* copy from mtk_internal.h */
#define RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT	(SIOCIWFIRSTPRIV + 0x1F)

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
	char macaddr[MAX_CLIENT_NUM][MAC_ADDR_STRING_LEN];
	int clientCnt;
} SNMP_CLIENT_TABLE_DATA;

/* copy from wireless driver */
typedef union _MACHTTRANSMIT_SETTING {
	struct {
		unsigned short MCS:7;	/* MCS */
		unsigned short BW:1;	/*channel bandwidth 20MHz or 40 MHz */
		unsigned short ShortGI:1;
		unsigned short STBC:2;	/*SPACE */
		unsigned short rsv:3;
		unsigned short MODE:2;	/* Use definition MODE_xxx. */
	} field;
	unsigned short word;
} MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;

typedef struct _RT_802_11_MAC_ENTRY {
	unsigned char ApIdx;
	unsigned char Addr[MAC_ADDR_SIZE];
	unsigned char Aid;
	unsigned char Psm;		/* 0:PWR_ACTIVE, 1:PWR_SAVE */
	unsigned char MimoPs;		/* 0:MMPS_STATIC, 1:MMPS_DYNAMIC, 3:MMPS_Enabled */
	char AvgRssi0;
	char AvgRssi1;
	char AvgRssi2;
	unsigned int ConnectedTime;
	MACHTTRANSMIT_SETTING TxRate;
	unsigned int LastRxRate;
	short StreamSnr[3];				/* BF SNR from RXWI. Units=0.25 dB. 22 dB offset removed */
	short SoundingRespSnr[3];			/* SNR from Sounding Response. Units=0.25 dB. 22 dB offset removed */
} RT_802_11_MAC_ENTRY;

typedef struct _RT_802_11_MAC_TABLE {
	unsigned long Num;
	RT_802_11_MAC_ENTRY Entry[MAX_STA_PER_RADIO];
} RT_802_11_MAC_TABLE;

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
static UINT8 l_buf[24*1024] = {0};

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

static void api_wlan_getVapName(UINT32 rid, UINT32 apid, char *pBuf)
{
	if (NULL == pBuf)
	{
		return;
	}

	snprintf(pBuf, VAP_NAME_LEN, "%s%d", (rid == 0) ? "ra":"rai", apid);
}

static int api_wlan_getAllVapIflist(VAP_IF_LIST *pList)
{
	int rid = 0;
	int apid = 0;
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
	for (rid = 0; rid < MAX_RADIO_NUM; rid++)
	{
		for (apid = 0; apid < MAX_VAP_PER_RADIO; apid++)
		{
			api_wlan_getVapName(rid, apid, ifname);
			/* check if vap exist and is up state */

			memset(&ifr, 0, sizeof(ifr));
			strncpy(ifr.ifr_name, ifname, IFNAMELEN);
			if (ioctl(l_sock, SIOCGIFFLAGS, &ifr) < 0)
			{
				//ifname noexit
				continue;
			}
			if(ifr.ifr_flags & IFF_RUNNING) 
			{
				pList->ifentry[num].apIdx = apid;
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
	struct iwreq iwr;
	UINT8* pPos = NULL;
	RT_802_11_MAC_TABLE *pMacTbl = NULL;
	int len;
	int i = 0, n = 0;
	long int uptime = 0;
	int staIdx = 0;
	VAP_IF_LIST iflist;

	if (NULL == pData)
	{
		return ;
	}

	if (0 != _initSock())
	{
		return ;
	}

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
		memset(&iwr, 0, sizeof(iwr));
		memset(&l_buf, 0, sizeof(l_buf));
		strncpy(iwr.ifr_ifrn.ifrn_name, iflist.ifentry[i].ifname, sizeof(iwr.ifr_ifrn.ifrn_name));
		iwr.ifr_ifrn.ifrn_name[sizeof(iwr.ifr_ifrn.ifrn_name) - 1] = '\0';
		iwr.u.data.pointer = (void *) l_buf;
		iwr.u.data.length = sizeof(l_buf);

		if (ioctl(l_sock, RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT, &iwr) < 0)
		{
			DEBUG_CLIENT("ioctl failed, reason: %s\n", strerror(errno));
			continue;
		}

		len = iwr.u.data.length;
		if (len < sizeof(RT_802_11_MAC_ENTRY))
		{
			DEBUG_CLIENT("vap:%s, sta info too short\n", iflist.ifentry[i].ifname);
			continue;
		}

		pMacTbl = (RT_802_11_MAC_TABLE *)l_buf;
		if (pMacTbl->Num > MAX_STA_PER_RADIO)
		{
			DEBUG_CLIENT("get sta number is out of range!");
			continue;
		}

		for (staIdx = 0; staIdx < pMacTbl->Num; staIdx++)
		{
			if (n < MAX_STA_PER_RADIO)
			{
				memcpy((char*)pData->macaddr[n], l_ieee80211_ntoa(pMacTbl->Entry[staIdx].Addr, 0), MAC_ADDR_STRING_LEN);
				DEBUG_CLIENT("sta: %s\n", pData->macaddr[n]);

				n++;
			}
		}
	}
	pData->clientCnt= n;

	return ;
}

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/



