/************************************************************
Copyright (C) 2010-2011 TP-LINK TECHNOLOGIES CO., LTD.
File name	: tp_ipc.c
Version		: 1.0
Description	: ipc file for dhcp and httpd
Author		: ZQQ
Create date	: 16Mar2011
History:
---------------------------------------------------------------
1. 16Mar2011 ZQQ Create the file 
****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/socket.h>
#include "msgq.h"
#include "tp_ipc.h"


#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "utility_lib.h"
#include "unix_sock.h"

#include <netdb.h> /* getaddrinfo() */
#include <arpa/inet.h> /* inet_ntop() */
#include <net/if.h> /* if_nametoindex() */

#include <netinet/in.h>
#include "ifaddrs.h"
#include <netinet/icmp6.h>


static int msqid = 0;	/*消息队列的标识符*/
//static fd_set rfds;
static unsigned char rcvBuf[MSG_MAX_LEN + 4] = {0};
static int sockfd = 0;

#define ERROR -1
#define OK 1

#define IPV6_ADDR_PATH            "/proc/net/if_inet6"
#define DEFAULT_GATEWAY_PATH      "/proc/sys/net/ipv6/conf/br0/default_gateway"
#define DEFAULT_PREFIX_LEN_PATH   "/proc/sys/net/ipv6/conf/br0/default_prefix_len"
#define STR_LENGTH                256
/* pData to send msg_center */
static LANV6_DHCPV6C_FEEDBACK l_dhcpv6c_feedback;
static MSG_CENTER_MSG l_msgCenter;
static struct in6_addr l_link_lical_addr6;

/* ******************** local function ************************** */
/*                                                                */
/* ******************** local function ************************** */
static int dhcp6c_get_if_link_local_addr6(const char *pIfname, struct in6_addr *pLinkLocalAddr6)
{   
    struct ifaddrs *pIfaddr    = NULL;
	struct ifaddrs *pInterface = NULL;   
    struct in6_addr *pTmpAddr  = NULL;
    int ok = 0;
	int i = 0;

    if( (!pIfname) || (!pLinkLocalAddr6) )
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "parameter is NULL\r\n");
        return -1;
	}

    if (-1 == getifaddrs(&pIfaddr))
    {
        UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "can not get ifaddrs\r\n");
        return -1; 
    }

    for(pInterface = pIfaddr; pInterface != NULL; pInterface = pInterface->ifa_next) 
    {
        if( pInterface->ifa_name && (!strcmp(pInterface->ifa_name, pIfname)) && 
            (AF_INET6 == pInterface->ifa_addr && pInterface->ifa_addr->sa_family) )
        {
            pTmpAddr = &((struct sockaddr_in6 *)(pInterface->ifa_addr))->sin6_addr;
			
            if(IN6_IS_ADDR_LINKLOCAL(pTmpAddr))
            {
				memcpy(pLinkLocalAddr6, pTmpAddr, sizeof(struct in6_addr));
                ok = 1;
            }
        }
    }   

    freeifaddrs(pIfaddr);  
    if(!ok)
    {
        return -1;
    }    
    return 0;
}


static int dhcp6c_executeCmdAndGetRes(const char *pCmd, char *pResult)   
{   
    char buffer[STR_LENGTH] = {0};
    char cmd[STR_LENGTH]    = {0};
    FILE *pFile             = NULL;

    if ( (NULL == pCmd) || (NULL == pResult) )
    {
        printf("error: %s %s %d parameters are NULL \r\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }

    strncpy(cmd, pCmd, STR_LENGTH);

    if((pFile = popen(cmd, "r")) != NULL)   
    {   
        while(fgets(buffer, STR_LENGTH, pFile) != NULL)   
        {   
            strncat(pResult, buffer, strlen(buffer));   
            if(strlen(pResult) > STR_LENGTH)
            {
                break;
            }

        }   
        pclose(pFile);   
        pFile = NULL;   
    }   
    else  
    {   
        printf("popen %s error\n", cmd);
		return -1;
    }

    return 0;
}

/*
 * return:
 *      1: ipv6 地址全为 0 或者传入参数为空
 *      0: ipv6 地址不全为 0
 */
static int dhcp6c_ipv6_is_zero(const struct in6_addr* p_ipv6_addr)
{
	if (NULL == p_ipv6_addr)
	{
		return 1;
	}
	return (p_ipv6_addr->s6_addr32[0] | p_ipv6_addr->s6_addr32[1] |
		p_ipv6_addr->s6_addr32[2] | p_ipv6_addr->s6_addr32[3]) == 0;
}

/* ******************** public function ************************** */
/*                                                                 */
/* ******************** public function ************************** */

void dhcp6c_init()
{
	dhcp6c_get_if_link_local_addr6(BRIDGE_NAME, &l_link_lical_addr6);
}

void dhcp6c_get_lanv6_dhcpv6c_feedback(LANV6_DHCPV6C_FEEDBACK *pDhcpv6c_feedback)
{
    if (NULL == pDhcpv6c_feedback)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "pDhcpv6c_feedback is NULL\r\n");
        return;
    }

    memcpy(pDhcpv6c_feedback, &l_dhcpv6c_feedback, sizeof(LANV6_DHCPV6C_FEEDBACK));
}


void dhcp6c_update_msg_info()
{
	int ret = -1;

    memcpy(&l_msgCenter.content, &l_dhcpv6c_feedback, sizeof(LANV6_DHCPV6C_FEEDBACK));
    l_msgCenter.len = sizeof(LANV6_DHCPV6C_FEEDBACK);
    l_msgCenter.opId = UCL_OPID_LANV6_DHCPV6C_FEEDBACK;

    ret = unix_sock_sendDataToSrvWithoutResp(DHCPV6C_MODULE_NAME, UNIX_SOCK_ID_MSG_CENTER, 
									   &l_msgCenter, sizeof(l_msgCenter), UNIX_MSG_CENTER_MSG);

	if (ret != 0)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "send msg to msg-center failed.\r\n");
		return;
	}
}


void dhcp6c_set_init_lanv6_dhcpv6c_feedback(const DHCPV6C_MSG *pMsg)
{
    if (NULL == pMsg)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "parameter pMsg is NULL.\r\n");
        return;
    }

    switch (pMsg->cmd)
    {
        case DHCPCV6_IPV6C_CMD_STATIC:
            l_dhcpv6c_feedback.connType = LANV6_CONNTYPE_STATIC;
            break;

        case DHCPCV6_IPV6C_CMD_AUTOMATICALLY:
            l_dhcpv6c_feedback.connType = LANV6_CONNTYPE_DYNAMIC;
            break;
    }
}


void dhcp6c_set_ipv6_addr(const struct in6_addr *pAddr)
{
    if (NULL == pAddr)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "parameter pAddr is NULL.\r\n");
	}
	else
    {
        memcpy(&l_dhcpv6c_feedback.ipv6Addr, pAddr, sizeof(struct in6_addr));
    }
}


void dhcp6c_reset_ipv6_info()
{
	memset(&l_dhcpv6c_feedback, 0, sizeof(LANV6_DHCPV6C_FEEDBACK));
}


int dhcp6c_get_file_first_ch(const char *pFileName, char *pResult)
{
	char result[STR_LENGTH] = {0};
	char cmd[STR_LENGTH]    = {0};
	int ret = -1;

	if ( (NULL == pFileName) || (NULL == pResult) )
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "parameter is NULL.\r\n");
		return -1;
	}

	snprintf(cmd, STR_LENGTH, "cat %s", pFileName);

	ret = dhcp6c_executeCmdAndGetRes(cmd, result);
	if (ret < 0)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "get the first charameter of file failed.\r\n");
		return -1;
	}

	*pResult = result[0];

    return 0;
}


void dhcp6c_set_ipv6_prefix_len()
{
	int ret = -1;
	char prefix_char = '\0';

	ret = dhcp6c_get_file_first_ch(DEFAULT_PREFIX_LEN_PATH, &prefix_char);
	if (ret < 0)
	{
		/* 如果读取失败，置为默认值 */
		l_dhcpv6c_feedback.prefixLen = DEFAULT_PREFIX_LEN;
		return;
	}

	l_dhcpv6c_feedback.prefixLen = prefix_char - '\0';
}


int dhcp6c_get_ipv6_prefix_len()
{
	return l_dhcpv6c_feedback.prefixLen;
}

void dhcp6c_set_ipv6_gateway()
{
	FILE* pFile = NULL;
	struct in6_addr gateway;
	char default_gateway[INET6_ADDRSTRLEN];

	memset(&gateway, 0, sizeof(struct in6_addr));
	memset(default_gateway, 0, INET6_ADDRSTRLEN);

	if (access(DEFAULT_GATEWAY_PATH, 0) == 0)
	{
		pFile = fopen(DEFAULT_GATEWAY_PATH, "r");
		if (NULL == pFile)
		{
			UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "default_gateway file open failed.\r\n");
			return;
		}

		fscanf(pFile, "%s", default_gateway);
		
		fclose(pFile);
	}

	if (strlen(default_gateway) > 0)
	{
		int ret = util_str_to_ipv6_addr(default_gateway, &gateway);
		if (ret < 0)
		{
			UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "transfer gateway failed.\r\n");
			return;
		}
	}
    
    memcpy(&l_dhcpv6c_feedback.gateway, &gateway, sizeof(struct in6_addr));
}


void dhcp6c_set_ipv6_dns(const unsigned char *pDns1, const unsigned char *pDns2)
{
    if(NULL != pDns1)
	{
        util_str_to_ipv6_addr(pDns1, &l_dhcpv6c_feedback.preferredDNS);
	}
	else
	{
		UTILDBG_DEBUG(DHCPV6C_MODULE_NAME, "pDns1 is NULL\r\n");
	}

	if(NULL != pDns2)
	{
		util_str_to_ipv6_addr(pDns2, &l_dhcpv6c_feedback.alternateDNS);
	}
	else
	{
		UTILDBG_DEBUG(DHCPV6C_MODULE_NAME, "pDns2 is NULL\r\n");
	}

}


int dhcp6c_create_dir(const char *pDirName)
{
	char cmd[STR_LENGTH] = {0};

	if (NULL == pDirName)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "pDirName is NULL, create dir failed.\r\n");
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, STR_LENGTH, "mkdir -p %s", pDirName);
    util_execFormatCmd(cmd);

    return 0;
}


int dhcp6c_create_file_stateful_dhcp6c_conf(const char *pDirName) 
{
    const char *pData =  "interface br0 { send ia-na 1; request domain-name-servers; request domain-name;}; id-assoc na 1{ };";
    const char *pFileName = DHCPV6_STATEFUL_DHCPV6_CONF_FILE;
    int ret = -1;
	FILE* pFile = NULL;
    
    char file_path[DHCPV6_MAX_PATH_LENGTH];
    memset(file_path, 0, DHCPV6_MAX_PATH_LENGTH);
    snprintf(file_path, DHCPV6_MAX_PATH_LENGTH, "%s/%s", pDirName, pFileName);

    if (NULL == pDirName)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "pDirName is NULL, create dir failed.\r\n");
		return -1;
	}

    /* if direction exists */
    if((access(pDirName, 0)) < 0)
    {   
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "file not exist.\r\n");
        return -1;
    }

    pFile = fopen(file_path, "w");
	if (NULL == pFile)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "open file failed.\r\n");
        return -1;
	}
    
    ret = fputs(pData, pFile);
    if (ret < 0)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "write data to file failed.\r\n");
        fclose(pFile);
        return -1;
    }

    fclose(pFile);
}



/*
    brief: create stateful_dhcp6c.conf in direction named pDirName
    param[in]: pDirName, pFileName
    return: 
        -1 : failed
        0  : success 
*/
int dhcp6c_create_file_information_dhcp6c_conf(const char *pDirName) 
{
    const char *pData =  "interface br0 { information-only;}; ";
    const char *pFileName = DHCPV6_INFORMATION_CONF_FILE;
    int ret = -1;
	FILE* pFile = NULL;
    
    char file_path[DHCPV6_MAX_PATH_LENGTH];
    memset(file_path, 0, DHCPV6_MAX_PATH_LENGTH);
    snprintf(file_path, DHCPV6_MAX_PATH_LENGTH, "%s/%s", pDirName, pFileName);

    if (NULL == pDirName)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "pDirName is NULL, create dir failed.\r\n");
		return -1;
	}

    /* if direction exists */
    if((access(pDirName, 0)) < 0)
    {   
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "file not exist.\r\n");
        return -1;
    }

	pFile = fopen(file_path, "w");
	if (NULL == pFile)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "open file failed.\r\n");
        return -1;
	}
    
    ret = fputs(pData, pFile);
    if (ret < 0)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "write data to file failed.\r\n");
        fclose(pFile);
        return -1;
    }

    fclose(pFile);
}


/* 项目里 ifterface_name 是 br0 */
void dhcp6c_open_slaac(const char *pInterfaceName)
{
    char command_slaac[STR_LENGTH] = {0};
    char command_autoconf[STR_LENGTH] = {0};

    if (NULL == pInterfaceName)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "parameter pInterfaceName is NULL\r\n");
        return;
    }

    snprintf(command_slaac, STR_LENGTH, "echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", pInterfaceName);
    snprintf(command_autoconf, STR_LENGTH, "echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", pInterfaceName);

    util_execFormatCmd(command_slaac);
    util_execFormatCmd(command_autoconf);

}


/* 项目里 ifterface_name 是 br0 */
void dhcp6c_close_slaac(const char *pInterfaceName)
{
    int ret = -1;

    if (NULL == pInterfaceName)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "parameter pInterfaceName is NULL\r\n");
        return;
    }

    char command_slaac[STR_LENGTH];
    char command_autoconf[STR_LENGTH];

    snprintf(command_slaac, STR_LENGTH, "echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", pInterfaceName);
    snprintf(command_autoconf, STR_LENGTH, "echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", pInterfaceName);

    ret = util_execFormatCmd(command_slaac);
    if (ret == -1)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "echo 2 > /proc/.../accept_ra fialed\r\n");
        return;
    }

    ret = util_execFormatCmd(command_autoconf);
    if (ret == -1)
    {
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "echo 2 > /proc/.../autoconf fialed\r\n");
        return;
    }

}


/*
	获取文件的一个值
	return：
		成功： 返回正确值
		失败： 返回 -1
*/
int dhcp6c_get_config_file_value(const char *pFileName)
{
	char result[STR_LENGTH] = {0};
	char cmd[STR_LENGTH]    = {0};
	int ret = -1;

	if (NULL == pFileName)
	{
		ipc_msg("parameter is NULL");
		return -1;
	}

	snprintf(cmd, STR_LENGTH, "cat %s", pFileName);

	ret = dhcp6c_executeCmdAndGetRes(cmd, result);
	if (ret < 0)
	{
		ipc_msg("get value failed");
		return -1;
	}

    return result[0] - '0';
}

int dhcp6c_get_mbit()
{
	return dhcp6c_get_config_file_value("/proc/sys/net/ipv6/conf/br0/ndisc_mbit");
}

int dhcp6c_get_obit()
{
	return dhcp6c_get_config_file_value("/proc/sys/net/ipv6/conf/br0/ndisc_obit");
}

/*
	0: invalid
	1: valid
*/
int dhcp6c_check_m_o_isvalid()
{
	int m_bit = -1;
	int o_bit = -1;

	m_bit = dhcp6c_get_mbit();
	o_bit = dhcp6c_get_obit();

	if (1 == m_bit && 0 == o_bit)
	{
		return 0;
	}
	else if (1 == m_bit && 1 == o_bit)
	{
		return 1;
	}
	else if (0 == m_bit && 1 == o_bit)
	{
		return 1;
	}
	else if (0 == m_bit && 0 == o_bit)
	{
		return 1;
	}

	return 0;
}

void dhcp6c_get_only_ipv6_addr_from_if_inet6(struct in6_addr *pAddr, const char *pInterfaceName)
{
	char addr6[40] = {0};
	char devname[20] = {0};
	int plen, scope, dad_status, if_idx;
	char addr6p[8][5];
    char cmd[STR_LENGTH] = {0};
	struct in6_addr ipv6_addr;

	if ( (NULL == pInterfaceName) || (NULL == pAddr) )
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "paramenters is NULL.\r\n");
		return;
	}

    FILE* pFile = fopen(IPV6_ADDR_PATH, "r");
	if (NULL == pFile)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "open file failed.\r\n");
		return;
	}

    while (fscanf
		   (pFile, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
			addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
			addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
			&dad_status, devname) != EOF)
    {
        if (!strcmp(devname, pInterfaceName))
        {
            snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);

			memset(&ipv6_addr, 0, sizeof(struct in6_addr));
			util_str_to_ipv6_addr(addr6, &ipv6_addr);

			if(!IN6_IS_ADDR_LINKLOCAL(&ipv6_addr))
            {
				memcpy(pAddr, &ipv6_addr, sizeof(struct in6_addr));
            }
        }
    }

    fclose(pFile);   
}


void dhcp6c_delete_ipv6_except_link_local(const char *pInterfaceName)
{
    char addr6[40] = {0}; 
	char devname[20] = {0};
	int plen, scope, dad_status, if_idx;
	char addr6p[8][5];
    char cmd[STR_LENGTH] = {0};
	struct in6_addr ipv6_addr;

	if (NULL == pInterfaceName)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "paramenters is NULL.\r\n");
		return;
	}

    FILE* pFile = fopen(IPV6_ADDR_PATH, "r");
	if (NULL == pFile)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "open file failed.\r\n");
		return;
	}

    while (fscanf
		   (pFile, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
			addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
			addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
			&dad_status, devname) != EOF)
    {
        if (!strcmp(pInterfaceName, devname))
        {
            snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);

			memset(&ipv6_addr, 0, sizeof(struct in6_addr));
			util_str_to_ipv6_addr(addr6, &ipv6_addr);

			if (!IN6_IS_ADDR_LINKLOCAL(&ipv6_addr))
			{
				snprintf(cmd, sizeof(cmd), "ifconfig %s del %s/%d", pInterfaceName, addr6, plen);
				util_execFormatCmd(cmd);
				ipc_msg("info: %s", cmd);
			}
        }
    }

    fclose(pFile);    
}

void dhcp6c_send_rs(const char *pInterfaceName)
{
	char cmd_buf[STR_LENGTH] = {0};

	if (NULL == pInterfaceName)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "paramenters is NULL.\r\n");
		return;
	}

	memset(cmd_buf, 0, sizeof(cmd_buf));
	snprintf(cmd_buf, sizeof(cmd_buf), "/proc/sys/net/ipv6/conf/%s/sendrs", pInterfaceName);
	if (access(cmd_buf, F_OK) != 0)
	{
		return;
	}

	memset(cmd_buf, 0, sizeof(cmd_buf));
	snprintf(cmd_buf, sizeof(cmd_buf), "echo 1 > /proc/sys/net/ipv6/conf/%s/sendrs", pInterfaceName);
	util_execFormatCmd(cmd_buf);
}

void dhcp6c_set_invalid_m_o()
{
	util_execFormatCmd("echo 1 > /proc/sys/net/ipv6/conf/br0/ndisc_mbit");
	util_execFormatCmd("echo 0 > /proc/sys/net/ipv6/conf/br0/ndisc_obit");
}


int dhcp6c_get_ipv6_number(const char *pIfname)
{
    int ipv6Num = 0;
	int plen = 0, scope = 0, dad_status = 0, if_idx = 0;
    char addr6[40] = {0};
	char devname[20] = {0};
	char addr6p[8][5];
	FILE *pFile = NULL;

	if (NULL == pIfname)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "paramenters is NULL.\r\n");
		return -1;
	}

    pFile = fopen(IPV6_ADDR_PATH, "r");
	if (NULL == pFile)
	{
		UTILDBG_ERROR(DHCPV6C_MODULE_NAME, "open file failed.\r\n");
		return -1;
	}

    while (fscanf
		   (pFile, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
			addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
			addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
			&dad_status, devname) != EOF)
    {
        if (!strcmp(devname, pIfname))
        {
            ipv6Num++;
        }
    }

    fclose(pFile);   

    return ipv6Num;
}

void printBuf(char *buf, int len)
{
	int i;
	unsigned char *p = buf;

	printf("buf = %08X, len=%d", (unsigned int) buf, len);
	for (i=0; i<len; i++, p++)
	{
		if (i % 16 == 0)
			printf("\n\r%02X ", *p);
		else if (i % 8 == 0)
			printf("- %02X ", *p);
		else
			printf("%02X ", *p);
	}
}



/* added by ZQQ, 12Apr2011 */
static dhcp6cInfo info;
/*end*/


int tp_udhcp_sp_setup()
{
	return udhcp_sp_setup();
}

int tp_udhcp_sp_fd_set(fd_set *pRfds)
{
	return udhcp_sp_fd_set(pRfds, -1);
}

void tp_dhcp_ipc_fork(int module_id)
{
	int cmd = 0;
	int res = 0;
	/* Init the status to -1, see 详解2, 262 by ZQQ, 30Nov11 */
	info.status = -1;
	
	sockfd = dhcp6c_ipc_socket();
	if (sockfd < 0)
	{
		ipc_msg("dhcp6c_ipc_socket error");
		return;
	}
	
	res = registerMid();
	if(res == ERROR)
	{
		ipc_msg("Register DHCPC errror");
		return;
	}
	
	int pid = fork();
	if (pid < 0)
	{
		perror("fork error:");
		exit(0);
	}
	
	if (pid == 0)	/*child*/
	{
		while(1)
		{	
			cmd = 0;
			cmd = dhcp6c_ipc_rcv(sockfd);
			if (cmd > 0)
			{
				ipc_msg("cmd = %d\r\n", cmd);
				udhcpv6_sp_ipc_inform(cmd);
			}
			sleep(1);
		}
		exit(0);
	}
}


int recv_httpd_cmd()
{
	struct timeval timeout;
	int retval = 0;
	int rdlen = 0;
	int cmd = 0;
	int max_fd = 0;
	int rdPipe = 0;
	fd_set rfds;
	#define TP_TIMEOUT_MAX 100*1000;	

	/*只监听1秒*/
	timeout.tv_sec = 0;
	timeout.tv_usec = TP_TIMEOUT_MAX;

	//ipc_msg("to get httpd cmd form child process");
	max_fd = tp_udhcp_sp_fd_set(&rfds);
	retval = select(max_fd + 1, &rfds, NULL, NULL, &timeout);
	//ipc_msg("max_fd = %d, retval = %d", max_fd, retval);
	if (-1 == retval)
	{
		perror("select error:");
		return -1;
	}
	else if(retval > 0)
	{
		rdPipe = getReadPipe();
		ipc_msg("rdPipe = %d",rdPipe);
		if(FD_ISSET(rdPipe, &rfds))
		{
			rdlen = read(rdPipe, &cmd, sizeof(int));
			if (rdlen < 0)
			{
				perror("read error:");
				return -1;
			}
			else
			{
				ipc_msg("cmd = %d\r\n", cmd);
				return cmd;
			}
		}
		else
		{
			//ipc_msg("FD_ISSET(rfd, &rfds) Not True\r\n");
		}
	}
	else
	{
		//ipc_msg("no data arrive within 1s.\r\n");
	}
	return -1;
}


int dhcp6c_ipc_socket()
{
	int client_sockfd;	
	int len;
	int res = 0;
	struct sockaddr_un server_sockaddr, cli_sockaddr;
	int result = 0;

	client_sockfd= socket(AF_UNIX, SOCK_STREAM, 0);
	cli_sockaddr.sun_family = AF_UNIX ;
	strcpy(cli_sockaddr.sun_path, IPC_DHCP6C_PATH);
	unlink(cli_sockaddr.sun_path);

	res = bind(client_sockfd, (struct sockaddr * )&cli_sockaddr, sizeof( cli_sockaddr ) ) ;	
	if(res < 0)
	{
		perror("bind error");
		return -1;
	}

	server_sockaddr.sun_family = AF_UNIX;	
	strcpy(server_sockaddr.sun_path, IPC_SERVER_PATH) ;	
	
	len=sizeof(server_sockaddr);	
	result = connect(client_sockfd,( struct sockaddr * )&server_sockaddr,len);	
	if (result < 0)	
	{		
		printf("ClientA::error on connecting \n");				
		return -1;	
	}	
	return client_sockfd;
}

int sendRegisterMsg(int sockfd, tp_ipc_module_id mid, int msgType)
{
	int len = 0;	
	UINT32 type = 0;	
	UINT8 buf[8] = {0};		

	type = (msgType == IPC_REGISTER) ? 0xFFFFFFFF : 0xEEEEEEEE;
	*(UINT32*)buf = type;	
	*(UINT32*)(buf + 4) = mid;	

	len = send(sockfd, buf, 8, 0);	
	if(len < 0)	
	{		
		perror("sendRegisterMsg error");
		return ERROR;	
	}	
	else 	
	{		
		if(msgType == IPC_UNREGISTER)
		{			
			close(sockfd);		
		}		
		return OK;	
	}
}

int dhcp6c_ipc_rcv(int sockfd)
{	
	int maxfd = 0;	
	int res = 0;	
	int nRead = 0;
	int cmd = 0;
	fd_set rset;	
	struct timeval tv;	
	tp_ipc_msg* pMsg = NULL;	
	UINT32* pType = NULL;	
	UINT8 rcvBuf[MSG_MAX_LEN] = {0};

	FD_ZERO(&rset);	
	tv.tv_sec = 0;	
	tv.tv_usec = 200*1000;
	
	do
	{		
		FD_SET(sockfd, &rset);		
		maxfd = sockfd + 1;		
		res = select(maxfd, &rset, NULL, NULL, &tv);
		
		if(res > 0)		
		{			
			if(FD_ISSET(sockfd, &rset))			
			{			
				ioctl(sockfd, FIONREAD, &nRead);
				//ipc_msg("nRead = %d", nRead);
				if(nRead <= 0)
				{
					return 0;
				}
					
				nRead = read(sockfd, rcvBuf, MSG_MAX_LEN);
				ipc_msg("nRead = %d", nRead);
				if(nRead < 0)		
				{				
					perror("read error");	
					continue;				
				}				
				else if(nRead > 0)				
				{					
					pMsg = (tp_ipc_msg*)(rcvBuf + 4);
					/* check the type */			
					pType = (UINT32 *)rcvBuf;
					//printf("Recv data\r\n");
					//printBuf(rcvBuf, nRead);
					//printf("\r\n");
					ipc_msg("*pType = %d, pMsg->srcMid=%d pMsg->dstMid = %d", *pType, pMsg->srcMid, pMsg->dstMid);
					if (*pType != DHCP6C || pMsg->dstMid != DHCP6C)	
					{			
						ipc_msg("IPC:this msg is not for DHCP6C\n");	
						return -2;				
					}					
					if (pMsg->magic != TP_IPC_MAGIC)
					{				
						ipc_msg("IPC:incorrect magic\n");	
						return -3;			
					}					
					if (pMsg->version != TP_IPC_VERSION)	
					{						
						ipc_msg("IPC:unsupported version\n");	
						return -4;				
					}							
					cmd = *(UINT8*)(&pMsg->payload[0]);
					ipc_msg("nRead = %d, cmd = %d", nRead, cmd);					
					return cmd;
				}			
			}
		}	
		else
		{
			return 0;
		}
	}while(0);
}


int dhcp6c_ipc_send(int dstMid, unsigned char* ptr, int nbytes)
{
	int nSnd = 0;
	unsigned char sendBuf[MSG_MAX_LEN] = {0};	
	tp_ipc_msg* pMsg = (tp_ipc_msg*)(sendBuf + 4);
	*((UINT32*)sendBuf) = dstMid;		/* msg type:unix ipc method requested */
	pMsg->magic = TP_IPC_MAGIC;	
	pMsg->version = TP_IPC_VERSION;
	pMsg->dstMid = dstMid;
	pMsg->srcMid = DHCP6C;
	pMsg->msgType = 0; 				/*暂不使用*/
	pMsg->bFrag = 0;				/* httpd_ipc_send not support fragment now */	
	if (nbytes > MSG_MAX_LEN - sizeof(tp_ipc_msg))	
	{		
		ipc_msg("dhcp_ipc_send: msg too log\n");	
		return -1;	
	}	
	memcpy(pMsg->payload, ptr, nbytes);
	nSnd = send(sockfd, sendBuf, nbytes + sizeof(tp_ipc_msg) + 4, MSG_DONTWAIT);
	if(nSnd >= 0)
	{
		ipc_msg("send %d byte success.", nSnd);
	}
	if (nSnd < 0)	
	{		
		ipc_msg("httpd_ipc_send: msgsnd");
		perror("send error:");
		return -2;	
	}	
	return 0;
}

int update_info()
{
	int res = 0;
	//printf("update_info:\r\n");
	//printBuf(&info, sizeof(dhcp6cInfo));
	res = dhcp6c_ipc_send(HTTPD, (UINT8*)&info, sizeof(dhcp6cInfo));
	if (res < 0)
	{
		ipc_msg("update_info error");
		return ERROR;
	}
	return OK;
}

void setStatus(int status)
{
	ipc_msg("%s %d set info.status = %d", __FUNCTION__, __LINE__, status);
	info.status = status;
}

void setType(int type)
{
	ipc_msg("%s %d set info.type = %d\r\n", __FUNCTION__, __LINE__, type);
	info.type = type;
}


void setIp(unsigned char* pv6Ip)
{
	ipc_msg("%s %d set ip\r\n", __FUNCTION__,__LINE__);
	if(pv6Ip)
	{
		memcpy(info.v6ip, pv6Ip, 40);
		ipc_msg("%s %d info.v6ip = %s\r\n", __FUNCTION__,__LINE__, info.v6ip);
	}
	else
	{
		ipc_msg("pv6Ip == NULL");
	}
}



void setPrefix(unsigned char* pv6Prefix, int prefixLength)
{
	ipc_msg("%s %d set prefix = %s, prefixLength = %d\r\n", __FUNCTION__, __LINE__, pv6Prefix, prefixLength);
	if (pv6Prefix)
	{
		memcpy(info.v6prefix, pv6Prefix, 40);
		info.prefixLen = prefixLength;
	}
	else
	{
		ipc_msg("pv6Prefix == NULL");
	}
}

void setDns(unsigned char* pDns1, unsigned char* pDns2)
{
	printf("%s %d set Dns\r\n", __FUNCTION__,__LINE__);
	if(pDns1 != NULL)
	{
		memcpy(&info.v6dns[0][0], pDns1, 40);	
	}

	if(pDns2 != NULL)
	{
		memcpy(&info.v6dns[1][0], pDns2, 40);	
	}
}

void resetDhcp6cInfo()
{
	memset(&info, 0, sizeof(info));
}

int unRegisterMid()
{
	int res;
	res = sendRegisterMsg(sockfd, DHCP6C, IPC_UNREGISTER);
	return res;
}
int registerMid()
{
	int res;
	res = sendRegisterMsg(sockfd, DHCP6C, IPC_REGISTER);
	return res;
}

