#ifndef CGIPOSTHANDLE_HPP
# define CGIPOSTHANDLE_HPP

#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fstream> // Add this line to include the <fstream> header file
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>

class CgiPostHandle {
	public:
		CgiPostHandle(void);
		~CgiPostHandle(void);
		size_t findContentlength(char *buffer);
		std::string setContentType(std::string line);
		std::string setFilename(std::string response);
		size_t findBytesWrite(std::string temp);
		std::string findBoundary(std::string line);
		size_t findBytesStart(char *buffer, size_t len);
		size_t createFile(char *buffer, int sockfd, int bytes_received);
		void setSockfd(int fd1, int fd2);
		void setBoundary(std::string boundary);
		int getReadfd(void);
		int getWritefd(void);
		void setFilebuffer(char *buffer);
		int getSockfd(void);
		char *getlastestFilebuffer(void);
		size_t getContentLength(void);
		size_t getBytesWrite(void);
		size_t getBytesRead(void);
		size_t getResponseLength(void);
		std::string getResponse(const char *msg);
		std::string getFilename(void);

	private:
		std::string _boundary;
		std::string _filename;
		std::string _contentType;
		std::string _response;
		size_t _contentLength;
		size_t _responseLength;
		size_t _bytesWrite;
		size_t _bytesRead;
		int _readfd;
		int _writefd;
		std::vector<char *> _filebuffer;
};

#endif