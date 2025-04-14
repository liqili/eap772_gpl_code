/************************************************************
Copyright (C) 2010-2011 TP-LINK TECHNOLOGIES CO., LTD.
File name	: tp_ipc.h
Version		: 1.0
Description	: Head file of tp_ipc.c
Author		: ZQQ
Create date	: 16Mar2011
History:
---------------------------------------------------------------
1. 16Mar2011 ZQQ Create the file 
****************************************************************/
#ifndef _TP_IPC_H_
#define _TP_IPC_H_

#include "opLanv6_info.h"
#include "unix_sock_msgDef.h"
#include "utility_debug.h"

#define DEFAULT_PREFIX_LEN                 (64)
#define BRIDGE_NAME                        "br0"
#define DHCPV6C_MODULE_NAME                "wide-dhcpv6"
/* 创建 dhcp6c 启动的配置文件 */
#define DHCPV6_MAX_PATH_LENGTH             (128)
#define DHCPV6_CONF_DIR                    "/tmp/wide-dhcpv6"
/* file name */
#define DHCPV6_INFORMATION_CONF_FILE       "information_dhcp6c.conf"
#define DHCPV6_STATEFUL_DHCPV6_CONF_FILE   "stateful_dhcp6c.conf"
#define DHCPV6_DHCP6CCTLKEY_FILE           "dhcp6cctlkey"
/* file path */
#define DHCPV6_INFORMATION_CONF_PATH       "/tmp/wide-dhcpv6/information_dhcp6c.conf"
#define DHCPV6_STATEFUL_CONF_PATH          "/tmp/wide-dhcpv6/stateful_dhcp6c.conf"

void dhcp6c_init();
void dhcp6c_get_lanv6_dhcpv6c_feedback(LANV6_DHCPV6C_FEEDBACK *pDhcp6cFeedback);
void dhcp6c_update_msg_info();
void dhcp6c_set_init_lanv6_dhcpv6c_feedback(const DHCPV6C_MSG *pMsg);
void dhcp6c_set_ipv6_addr(const struct in6_addr *pAddr);
void dhcp6c_set_ipv6_gateway();
void dhcp6c_set_ipv6_prefix_len();
void dhcp6c_reset_ipv6_info();
int dhcp6c_get_ipv6_prefix_len();
void dhcp6c_set_ipv6_dns(const unsigned char *pDns1, const unsigned char *pDns2);
int dhcp6c_create_dir(const char *pDirName);
int dhcp6c_create_file_stateful_dhcp6c_conf(const char *pDirName);
int dhcp6c_get_config_file_value(const char *pFileName);
void dhcp6c_close_slaac(const char *pIfName);
void dhcp6c_open_slaac(const char *pIfName);
int dhcp6c_create_file_information_dhcp6c_conf(const char *pDirName);
void dhcp6c_get_only_ipv6_addr_from_if_inet6(struct in6_addr *pAddr, const char *pIfName);
void dhcp6c_delete_ipv6_except_link_local(const char *pIfName);
void dhcp6c_send_rs(const char *pIfName);
int dhcp6c_check_m_o_isvalid();
void dhcp6c_set_invalid_m_o();
int dhcp6c_get_mbit();
int dhcp6c_get_obit();
int dhcp6c_get_ipv6_number(const char *pIfname);


#include "signalpipe.h"
typedef struct
{
	int	type;
	int status;
	unsigned char v6ip[40];
	unsigned char gw[40];
	unsigned char v6prefix[40];
	int prefixLen;
	unsigned char v6dns[2][40];
}dhcp6cInfo;

#define IPC_DEBUG
//#undef 	IPC_DEBUG
#ifdef  IPC_DEBUG
#define ipc_msg(...) printf("%s %s %d : ",__FILE__,__FUNCTION__,__LINE__); \
		printf(__VA_ARGS__); \
		printf("\r\n")
#else
#define ipc_msg(...)
#endif


int tp_ipc_init();
int dhcp6c_ipc_send(int dstMid, unsigned char* ptr, int nbytes);
int tp_udhcp_sp_fd_set(fd_set *pRfds);
int dhcp6c_ipc_rcv(int module_id);
void tp_dhcp_ipc_fork(int module_id);
int recv_httpd_cmd();
int update_info();
void setDns(unsigned char* pDns1, unsigned char* pDns2);
void setPrefix(unsigned char* pv6Prefix, int prefixLength);
void setIp(unsigned char* pv6Ip);
void setType(int type);
void setStatus(int status);

#endif
