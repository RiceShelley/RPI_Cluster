#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <cstdlib>
#include "Client.h"
#include "Log.h"

int sock;
int port = 9801;
int client;
// array of client objects
std::vector<Client*> clients;

int main()
{	
	std::cout << "NFT STARTED" << std::endl;
		
	Log* log = new Log((char*) "NFT.log");
	log->append("ran\n");
	
	// create socket
	struct sockaddr_in sockDef;
	
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockDef.sin_family = AF_INET;
	sockDef.sin_port = htons(port);
	sockDef.sin_addr.s_addr = INADDR_ANY;
	
	// bind socket
	if (bind(sock, (struct sockaddr*)&sockDef, sizeof(sockDef)) < 0)
	{
		std::cout << "NFT: failed to bind socket!\nNFT: exiting..." << std::endl;
		return 0;
	}

	listen(sock, 1);

	struct sockaddr_in clientDef;
	socklen_t clientln = sizeof(clientDef);

	// listen for clients
	while (true)
	{
		client = accept(sock, (struct sockaddr*)&clientDef, &clientln);

		// create client object
		Client* c = new Client(client);
		c->start();
		// add client to array of client objects
		clients.push_back(c);
		for (int i = 0; i < clients.size(); i++)
		{
			if (clients.at(i)->getConnState() == false)
			{
				clients.erase(clients.begin() + i);
			}
		}
	}
}
