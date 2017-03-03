/*
 * =====================================================================================
 *
 *       Filename:  Job.h
 *
 *    Description:  execute "jobs" on a seperate thread
 *
 *        Version:  1.0
 *        Created:  07/14/2016 06:29:02 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rice Shelley
 *   Organization:  NONE 
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

void create_job(char* cmd_p, char* ip_addr, int jRPort) {
	// alcolate data for new process before fork	
	char cmd[strlen(cmd_p) + 1];
	strcpy(cmd, cmd_p);
	pid_t pid = fork();
	if (pid == 0)
	{
		system(strcat(cmd, " > cmdOutput"));
		if (jRPort != -1)
		{
			// attempt connection to socket at suplied address 
			struct sockaddr_in server_addr;
			int sockID = socket(AF_INET, SOCK_STREAM, 0);
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(jRPort);
			if (inet_pton(AF_INET, (const char*) ip_addr, &server_addr.sin_addr.s_addr) <= 0)
			{
				printf("inet_pton error\n");
			}
			if (connect(sockID, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
			{
				perror("failed to connect to job server");
				close(sockID);
				exit(0);
			}
			// Read in output of cmd
			FILE* fp = fopen("cmdOutput", "r");
			char mem[2555];
			fgets(mem, 2555, fp);
			fclose(fp);
			// send output
			send(sockID, mem, strlen(mem), 0); 
			close(sockID);	
		}
		exit(0);
	}
}

