#ifndef FILEIO_H
#define FILEIO_H

#include <iostream>
#include <string.h>
#include <fstream>

class FileIO {

	public:
		/*
		 * append text to file given 
		 * path of file and text to append
		 */
		
		void appendFile(char path[260], std::string text);

		/*
		 * create a new file
		 * given a full path
		 */

		void wipeAndWrite(char path[260], std::string text);
		
		/* read file 
		 * given file path
		 */

		std::string readFile(char path[260]);		

		/*
		 * write binnary file
		 */	
		void writeBinaryFile(char path[260], char* data, int len);

		/*
		 * read in binary 
		 * file given path
		 */
		char* readAsBinary(char path[260], std::streampos *size);
};
#endif
