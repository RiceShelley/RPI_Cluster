#include "Log.h"

Log::Log(char* name)	
{
	logName = name;
	std::cout << "NFT: log created! at " << name << std::endl;
}

void Log::append(std::string data) {
	file.appendFile(logName, data);
}
