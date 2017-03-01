/*
 * =====================================================================================
 *
 *       Filename:  ServiceUtil.h
 *
 *    Description:  util functions for servicing clients
 *
 *        Version:  1.0
 *        Created:  07/13/2016 06:57:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rice Shelley
 *   Organization:  NONE 
 *
 * =====================================================================================
 */

#ifndef SERVICE_UTIL_H
#define SERVICE_UTIL_H
// ip4 max length + room for a null terminator
#define IPV4_MAXLEN 16

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

// IPC format struct  
struct IPC {
	char ip[IPV4_MAXLEN];
	char* port;
	char* cmd;
};

// clear memory @params pointer to memory
void memClear(char* ptr, int len) 
{
	memset(ptr, 0, len);
}

// note msg must be null terminated
int sendSimpleMsg(int conn, char* msg)
{
	if (strchr(msg, '\0') == NULL)
	{
		printf("no null terminator\n");
		strcat(msg, "\0");
	}
	send(conn, msg, strlen(msg), 0);
}

// Ip Port Cmd format parser <- IPC_Parse(char*) <- takes input in format of "ip:port cmd" 
struct IPC IPC_Parse(char* raw)
{
	struct IPC ipc;
	// get port
	ipc.port = strchr(raw, ':') + 1;
	// get ip addr
	int posOfBreak = (int) (strlen(raw) - strlen(ipc.port));
	memClear(ipc.ip, IPV4_MAXLEN);
	strncpy(ipc.ip, raw, (posOfBreak - 1));
	// get cmd 
	ipc.cmd = strchr(raw, ' ') + 1;
	return ipc;
}

#endif
