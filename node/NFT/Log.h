#ifndef LOG_H
#define LOG_H

#include <iostream>
#include "FileIO.h"

class Log {

 private:
	char *logName;
	FileIO file;
 public:
	 Log(char *name);
	//append to log file
	void append(std::string data);

};
#endif
