#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/select.h>
 #include <unistd.h>

#define UINT32 	unsigned int
#define UINT8	unsigned char
#define BOOL	int
#define TRUE	1
#define FALSE	0
#define OK		1
#define ERROR	-1
#define STATUS	int
#define TP_IPC_MAGIC		0xbabeface
#define TP_IPC_VERSION		0x10		/* 1.0 */
#define MSG_MAX_LEN			(2*1024 + 32)  /*64条路由条目和信息头的长度*/
#define MAX_SOCKET_LIST		20
#define SELECT_TIMEOUT		200*1000
#define IPC_SERVER_PATH		"/tmp/ipc"
#define IDLE_SOCKET			-1

#define IPC_DEBUG
#undef 	IPC_DEBUG
#ifdef  IPC_DEBUG
#define ipc_msg(...) printf("%s %s %d : ",__FILE__,__FUNCTION__,__LINE__); \
		printf(__VA_ARGS__); \
		printf("\r\n")
#else
#define ipc_msg(...)
#endif

typedef enum
{
	IDLE = 0,
	HTTPD = 1,
	PPPD = 2,
	PPPOE = 3,
	L2TP = 4,
	PPTP = 5,
	DHCPC = 6,
	DHCPS = 7,
	ADVSEC = 8,
	NTP = 9,
	DNS_DETECT = 10,
	SYS_STATS = 11,
	/*added by by ZQQ, Add the Processes, 21Mar2011 */
	DHCP6C = 12,
	DHCP6S = 13,
	RADVD =14,
	/*end added by ZQQ*/
	MODULE_ID_MAX = 0x100
}tp_ipc_module_id;

typedef struct tp_ipc_msg_struct
{
	UINT32 magic;		/* Must be 0xbabeface */
	UINT32 version;		/* Header version */
	UINT32 dstMid;		/* ID of receiver module */
	UINT32 srcMid;		/* ID of sender module */
	UINT32 msgType;		/* data type of the IPC msg */
	BOOL   bFrag;		/* There is fragment data in next msg */
	UINT8  payload[0];	/* real data */
}tp_ipc_msg;

struct clientinfo{
	tp_ipc_module_id mid;
	int cli_sockfd;
};

#ifdef  IPC_DEBUG
void printBuf(unsigned char *buf, int len)
{
	int i;
	unsigned char *p = buf;

	printf("buf = %08X, len=%d", (UINT32) buf, len);
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
#endif

int main(int argc,  char *argv[])
{
	struct sockaddr_un server;
	struct sockaddr_un client;
	int sockfd = 0;
	int newfd = 0;
	int serverLen = 0;
	socklen_t cliLen = 0;
	int ret = 0;
	int i = 0;
	int k = 0;
	int maxfd = 0;
	int nRead = 0;
	int unregSock = 0;
	UINT8 buf[MSG_MAX_LEN] = {0};
	UINT32 type = 0;
	tp_ipc_module_id mid = IDLE;
	tp_ipc_module_id dstMid = IDLE;
	tp_ipc_module_id srcMid = IDLE;
	struct  clientinfo clientInfoList[MAX_SOCKET_LIST];
	int cli_fd_list[MAX_SOCKET_LIST] = {-1};
	struct timeval tv;
	fd_set watchset;

	
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("socket error:");
		return ERROR;
	}
	memset(&server, 0, sizeof(server));
	for(i = 0; i < MAX_SOCKET_LIST; i++)
	{
		clientInfoList[i].mid = IDLE;
		clientInfoList[i].cli_sockfd = IDLE_SOCKET;
		cli_fd_list[i] = IDLE_SOCKET;
	}
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, IPC_SERVER_PATH);
	serverLen = sizeof(server);
	unlink(server.sun_path);
	
	ret = bind(sockfd,(struct sockaddr*)(&server),serverLen);
	if(ret < 0)
	{
		perror("bind error:");
		return ERROR;
	}
	
	ipc_msg("sockfd = %d",sockfd);
	ret = listen(sockfd, MAX_SOCKET_LIST);
	if(ret < 0)
	{
		perror("list error:");
		close(sockfd);
		return ERROR;
	}
	
	while(1)
	{
		/* clear watchset */
		FD_ZERO(&watchset);
		tv.tv_sec = 0;
		tv.tv_usec = SELECT_TIMEOUT;	/*200毫秒*/

		/* reset watchset */
		FD_SET(sockfd, &watchset);
		maxfd = sockfd;
		
		for(k = 0; k < MAX_SOCKET_LIST; k++)
		{
			if(cli_fd_list[k] != -1)
			{
				FD_SET(cli_fd_list[k], &watchset);
				maxfd = (maxfd < cli_fd_list[k]) ? cli_fd_list[k] : maxfd;
			}
		}
		
		ret = select(maxfd + 1, &watchset, NULL, NULL, &tv);
		if(ret < 0)
		{
			perror("select error:");
			continue;
		}
		else if (ret > 0)
		{
			/*判断是否有新的连接进入服务器*/
			if(FD_ISSET(sockfd, &watchset))
			{
				cliLen = sizeof(client);
				newfd = accept(sockfd, (struct sockaddr*)(&client), &cliLen);
				
				if (newfd < 0)
				{
					perror("accept error");
					continue;
				}
				
				memset(buf, 0, MSG_MAX_LEN);
				nRead = read(newfd, buf, MSG_MAX_LEN);
				type = *(UINT32*)(buf);
				
				/* client must register at first */
				if(type != 0xFFFFFFFF)
				{
					continue;
				}
				
				/*保存新的连接到数组中*/
				for(i = 0; i < MAX_SOCKET_LIST; i++)
				{
					if(cli_fd_list[i] == IDLE_SOCKET)
					{
						ipc_msg("add new socket %d to cli_fd_list[%d]", newfd, i);
						cli_fd_list[i] = newfd;
						break;
					}
				}
				
				mid = *(int*)(buf + 4);
				/*记录每个client 的进程标志和该进程和服务器通信使用的套接字*/
				for (i = 0; i < MAX_SOCKET_LIST; i++)
				{
					if(clientInfoList[i].mid == IDLE)
					{
						clientInfoList[i].mid = mid;	/* client process name */
						clientInfoList[i].cli_sockfd = newfd;	/* client socket */
						ipc_msg("add a clientInfoList[%d], socket = %d, MID = %d", i, clientInfoList[i].cli_sockfd, clientInfoList[i].mid);
						break;
					}
				}
				continue;
			}
			else 
			{
				for (i = 0; i < MAX_SOCKET_LIST; i++)
				{
					if(cli_fd_list[i] != IDLE_SOCKET && FD_ISSET(cli_fd_list[i], &watchset)) 
					{
						nRead = 0;
						/*先读出socket中的字符*/
						ioctl(cli_fd_list[i], FIONREAD, &nRead);
						if (nRead == 0)
						{
							continue;
						}  
						nRead = read(cli_fd_list[i], buf, MSG_MAX_LEN);
						type = *(UINT32*)buf;
						
						/*注销这个套接字*/
						if (type == 0xEEEEEEEE)
						{
							mid = *(int*)(buf + 4);
							for(i = 0; i < MAX_SOCKET_LIST; i++)
							{
								if(clientInfoList[i].mid == mid)
								{
									unregSock = clientInfoList[i].cli_sockfd;
									clientInfoList[i].cli_sockfd = IDLE_SOCKET;
									clientInfoList[i].mid = IDLE;
									ipc_msg("unregister mid:%d, socket:%d", mid, unregSock);
									for(k = 0; k < MAX_SOCKET_LIST; k++)
									{
										if(cli_fd_list[k] == unregSock)
										{
											ipc_msg("make cli_fd_list[%d] = IDLE_SOCKET", k);
											cli_fd_list[k] = IDLE_SOCKET;
										}
									}
									close(unregSock);
								}
							}
							continue;
						}
						
						srcMid = *(tp_ipc_module_id*)(buf + 16);
						dstMid = *(tp_ipc_module_id*)(buf + 12);						
						for(k = 0; k < MAX_SOCKET_LIST; k++)
						{
							if(dstMid == clientInfoList[k].mid)
							{
								/* forward msg to dst socket */
								send(clientInfoList[k].cli_sockfd, buf, nRead, MSG_DONTWAIT);
								break;
							}
						}
					}
				}
			}
		}
	}
}

