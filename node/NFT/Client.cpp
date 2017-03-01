#include "Client.h"

Client::Client(int sock) : clientID(sock)
{
	conn = true;
	strcpy(wDir, "../userBin/");
}

void Client::start()
{
	// Create POSIX thread
	pthread_create(&thread, NULL, Client::staticThreadEntryPoint, this);
}

// listen for clients
void Client::listen()
{
	while (true)
	{
		char fromC[128];
		memset(fromC, 0, 128);
        	read(clientID, fromC, 128);
		// ping server connection
		if (strcmp(fromC, "PING") == 0)
		{
			write(clientID, "PONG", 4);
		}
		// list files on server
		else if (strcmp(fromC, "LIST") == 0)
		{
			std::string files = listFiles();
			size_t fileSize = files.length();
			write(clientID, files.c_str(), fileSize);
		}
		// get file from server
		else if (strncmp(fromC, "GET ", 4) == 0)
		{
			std::string file(wDir);
			file += &fromC[4]; if (fileExist(file))
			{
				// let client know server has file
				write(clientID, "FILEEXIST", 9);
				std::streampos size;
				char* mem;
				char path[260];
				memset(path, 0, sizeof(path));
				strcat(path, wDir);
				strcat(path, &fromC[4]);
				mem = fileIO.readAsBinary(path, &size);
				// send binary file back to client
				write(clientID, mem, size);
				write(clientID, "<EOF>", 5);
			}
			else
			{
				write(clientID, "!FILEEXIST", 10);
			}
		}
		// Upload files to server storage
		else if (strncmp(fromC, "UPLOAD ", 7) == 0)
		{
			std::string fileName = &fromC[7];
			std::cout << "NFT: New file from Master: " << fileName << std::endl;
			recvFile(fileName);
		}
		// create new directory
		else if (strncmp(fromC, "MKDIR ", 6) == 0)
		{
			char path[256];
			strcpy(path, wDir);
			strcat(path, &fromC[6]);
			if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
			{
				std::cout << "NFT: new directroy at '" << path << "' created!" << std::endl;
			}
			else
			{
				std::cout << "NFT: Failed to create new directory!" << std::endl;
			}
		}
		// change working directory
		else if (strncmp(fromC, "CDIR ", 5) == 0)
		{
			char* dir = &fromC[5];
			int size = (sizeof(wDir) / sizeof(char));
			char wDirSave[size];
			strcpy(wDirSave, wDir);
			strcat(wDir, dir);
			strcat(wDir, "/");
			// check that dir is valid
			std::string files = listFiles();
			// if dir does not exist revert back to prev dir
			if (files == "NULL")
			{
				strcpy(wDir, wDirSave);
			}
		}
		else if (strncmp(fromC, "rm ", 3) == 0)
		{
			char* file = &fromC[3];
			char cmd[] = "rm ";
			char filePath[265];
			strcpy(filePath, (const char*) wDir);
			strcat(filePath, (const char*) file);
			system((const char*) strcat(cmd, (const char*) filePath));
		}
		// get current working directory
		else if (strcmp(fromC, "GETWDIR") == 0)
		{
			write(clientID, wDir, (sizeof(wDir) / sizeof(char)));
		}
		// set working directory to root dir
		else if (strcmp(fromC, "ROOTDIR") == 0)
		{
			memset(wDir, 0, (sizeof(wDir) / sizeof(char)));
			strcpy(wDir, "../userBin/");
		}
		// "" asociated with a dead client
		else if (strcmp(fromC, "") == 0)
		{
			close(clientID);
			break;
		}
		memset(fromC, 0, sizeof(fromC));
	}
}

void Client::recvFile(std::string fileName)
{
	int chunkSize = 1000;
	// create path of new file
	char path[260];
	memset(path, 0, 260);
	strcpy(path, wDir);
	strcat(path, fileName.c_str());
	// init data chunk
	char dataChunk[chunkSize];
	memset(dataChunk, 0, chunkSize);
	read(clientID, dataChunk, chunkSize);
	// create end tag
	char endTag[5];
	strcpy(endTag, "<EOF>");
	// chunks read
	while (true)
	{
		// sleep for half a second to avoid reading in null data from socket
		usleep(500);
		int endTagIndex = findSubStr(dataChunk, chunkSize, endTag, 5);
		// if end tag is in this chunk substring, write, and return
		if (endTagIndex != -1)
		{
			int endCSize = chunkSize - (chunkSize - endTagIndex);
			char endDataChunk[endCSize];
			for (int i = 0; i < endTagIndex; i++)
			{
				endDataChunk[i] = dataChunk[i];
			}
			fileIO.writeBinaryFile(path, endDataChunk, endCSize);
			break;
		}
		// if substring is not in this chunk write complete chunk to file
		else
		{
			fileIO.writeBinaryFile(path, dataChunk, chunkSize);
			memset(dataChunk, 0, chunkSize);
			read(clientID, dataChunk, chunkSize);
		}
	}
	std::string prompt = "File upload done\0";
	write(clientID, prompt.c_str(), strlen(prompt.c_str()));
}

// retuns true if file exist
bool Client::fileExist(const std::string& filePath)
{
	struct stat buffer;
	return (stat(filePath.c_str(), &buffer) == 0);
}

// find substring of char arrary returns index of start of substring
int Client::findSubStr(char* str, int strLen, char* find, int findLen)
{
	// mark marks the start of the substring
	int mark = 0;
	// keeps track of how many consecutive letters matching str[] in find[] have been found
	int found = 0;
	// iterate through str[]
	for (int i = 0; i < strLen; i++)
	{
		if (str[i] == find[found])
		{
			if (mark == 0)
			{
				mark = i;
			}
			found++;
		}
		else
		{
			mark = 0;
			found = 0;
		}
		if (found == findLen)
		{
			return mark;
		}
	}
	return -1;
}

std::string Client::listFiles()
{
	// gather all files in server storage and format thier names into one string
	std::string path(wDir);
	// formated list of files
	std::string files;
	DIR* dir;
	struct dirent* file;
	if ((dir = opendir(path.c_str())) == NULL)
	{
		std::cout << "NFT: ERROR could not locate storage!" << std::endl;
		files = "NULL";
	}
	else
	{
		while ((file = readdir(dir)) != NULL)
		{
			files += std::string(file->d_name) + " / ";
		}
	}
	closedir(dir);
	return files;
}

// static entry point for client thread
void* Client::staticThreadEntryPoint(void* c)
{
	Client* p = ((Client*)*(&c));
	p->listen();
	p->conn = false;
	return NULL;
}

bool Client::getConnState()
{
	return conn;
}
