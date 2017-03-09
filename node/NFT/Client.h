#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <cstdlib>
#include <string.h>
#include <dirent.h>
#include "FileIO.h"

class Client {

 private:
	int clientID;

	// security state of client
	bool authorized;

	// is state of client connection 
	bool conn;

	// thread reference stuff
	pthread_t thread;
	static void *staticThreadEntryPoint(void *c);

	// func maintain client
	void listen();

	// func for listing all files in storage 
	 std::string listFiles();

	// func for recving files from client
	void recvFile(std::string fileName);

	// find a substring of charecter array
	int findSubStr(char *str, int strLen, char *find, int findLen);

	// return true if file exist
	bool fileExist(const std::string & filePath);

	// working directory path
	char wDir[256];

	// FileIO object
	FileIO fileIO;
 public:
	// constructor
	 Client(int sock);

	// get client connection state
	bool getConnState();

	// call to start client thread
	void start();
};
#endif
