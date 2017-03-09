#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>

#include "ServiceUtil.h"
#include "Job.h"

// socket port number
const int port = 9800;

// node info struct
struct nodeInf {
	int nodeID;
	bool NFTstat;
};

// Func services clients request 
int service(int conn)
{
	const int fromC_ln = 256;
	char fromC[fromC_ln];
	while (true) {
		// Clear char buff and read from socket
		memClear(fromC, fromC_ln);
		read(conn, fromC, fromC_ln);
		// Server has been pinged
		if (strcmp(fromC, "PING") == 0) {
			send(conn, "PONG", 4, 0);
		}
		// cmd prefixed with "start " used to execute programs on node
		else if (strncmp(fromC, "start ", 6) == 0) {
			// Parse cmd in ipc format (ipc) = ip port cmd
			char *ipcRaw = &fromC[6];
			struct IPC ipc = IPC_Parse(ipcRaw);
			// NFT = Network file transpher
			if (strcmp(ipc.cmd, "NFT") == 0) {
				printf("NFT STARTED ON PORT 9801\n");
				create_job("cd bin && ./NFT\0", ipc.ip,
					   atoi((const char *)ipc.port));
				sendSimpleMsg(conn, "NFT STARTED ON PORT 9801");
			} else {
				printf("unkown start cmd '%s'", ipc.cmd);
				sendSimpleMsg(conn, "Unkown start cmd");
			}
		}
		// cmd prefixed with "kill " used to kill integrated programs on node
		else if (strncmp(fromC, "kill ", 5) == 0) {
			// Parse cmd in ipc format 
			char *ipcRaw = &fromC[5];
			struct IPC ipc = IPC_Parse(ipcRaw);
			if (strncmp(ipc.cmd, "NFT", 3) == 0) {
				create_job("pkill NFT", ipc.ip,
					   atoi((const char *)ipc.port));
				printf("kill job created for NFT\n");
				sendSimpleMsg(conn, "kill job created");
			} else {
				printf("unkown kill cmd '%s'\n", ipc.cmd);
				sendSimpleMsg(conn, "Unkown Kill cmd!!!");
			}
		}
		// run custom job via system() cmd
		else if (strncmp(fromC, "DOJOB ", 6) == 0) {
			// Parse cmd in ipc format
			char *ipcRaw = &fromC[6];
			struct IPC ipc = IPC_Parse(ipcRaw);
			// create job
			create_job(ipc.cmd, ipc.ip,
				   atoi((const char *)ipc.port));
		} else if (strcmp(fromC, "restart") == 0) {
			// restart node
			sendSimpleMsg(conn, "restarting node");
			return -1;
		} else if (strcmp(fromC, "shutdown") == 0) {
			// shutdown node
			sendSimpleMsg(conn, "shutting down node");
			return 1;
		}
		// if client is sending ""'s its beacuse of a lost connecton 
		else if (strcmp(fromC, "") == 0) {
			printf("client disconnect\n");
			return 0;
		} else if (strcmp(fromC, "help") == 0) {
			// read and send help file to user
			char fileCon[50][500];
			FILE *file = fopen("helpFile.txt", "r");
			int fCIndex = 0;
			do {
				fgets(fileCon[fCIndex], 500, file);
				if (strcmp(fileCon[fCIndex], "<EOF>\n") == 0)
					break;
				fCIndex++;
			}
			while (true);

			char content[500 * fCIndex];
			memset(content, 0, 500 * fCIndex);
			for (int i = 0; i < fCIndex; i++) {
				strcat(content, (const char *)fileCon[i]);
			}
			send(conn, content, 500 * fCIndex, 0);
		}
		printf("from client '%s'\n", fromC);
	}
}

int main()
{
	// Socket addr struct creation 
	struct sockaddr_in server_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// Create sock fd
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind socket to port
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("failed to bind socket to addr!\n");
		return -1;
	}
	// Listen for incoming connections
	listen(sock, 1);

	// Struct for client addr
	struct sockaddr_in connDef;
	socklen_t connDefLn = sizeof(connDef);
	while (true) {
		int conn =
		    accept(sock, (struct sockaddr *)&connDef, &connDefLn);
		// Manage client request in service()
		int exitStat = service(conn);
		if (exitStat == 0) {
			// Clean client dissconect go back to listing Mr. Socket
			continue;
		} else {
			// Bad client dissconnect return error code
			printf("Returning exit status: %d\n", exitStat);
			// -1 = restart node
			if (exitStat == -1) {
				return 0;
			}
			// Any other return value = restart nodeManager
			else {
				return exitStat;
			}
		}
	}
}
