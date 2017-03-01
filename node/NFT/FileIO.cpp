#include "FileIO.h"

void FileIO::appendFile(char path[260], std::string text) 
{
	if (std::ifstream(path)) 
	{	
		std::ofstream file;
		file.open(path, std::ios::app);
		file << text;
		file.close();
	} 
	else 
	{
		std::cout << "NFT: file " << path << " does not exist" << std::endl;
	}
}

void FileIO::wipeAndWrite(char path[260], std::string text)
{
	std::ofstream file;
	file.open(path, std::ios::out | std::ios::trunc);
	file << text;
	file.close();
}

std::string FileIO::readFile(char path[260]) 
{
	std::string data;	
	std::ifstream file;
	file.open(path);
	file >> data;
	file.close();
	return data;
}

void FileIO::writeBinaryFile(char path[260], char* data, int len)
{
	std::ofstream file(path, std::ios::out | std::ios::app | std::ios::binary);
	file.write(data, len);
	file.close();
}

char* FileIO::readAsBinary(char path[260], std::streampos *size)
{
	char *mem;
	std::ifstream file(path, std::ios::in|std::ios::binary|std::ios::ate);
	*size = file.tellg();
	mem = new char[*size];
	file.seekg(0, std::ios::beg);
	file.read(mem, *size);
	file.close();
	return mem;	
}
