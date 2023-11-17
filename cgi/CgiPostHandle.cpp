#include "CgiPostHandle.hpp"

const int BUFFER_SIZE = 100000;

CgiPostHandle::CgiPostHandle(void)
{
	_boundary = "";
	_filename = "";
	_contentType = "";
	_contentLength = 0;
	_bytesWrite = 0;
	// std::cout << "Default constructor called by <CgiPostHandle>" << std::endl;
}

CgiPostHandle::~CgiPostHandle(void)
{
	// std::cout << "Destructor called by <CgiPostHandle>" << std::endl;
}

size_t CgiPostHandle::findContentlength(char *buffer){
	std::istringstream ss(buffer);
	std::string line;
	size_t pos;
	while (std::getline(ss >> std::ws, line)) {
		pos = line.find("Content-Length:");
		if (pos != std::string::npos){
			std::istringstream num(line.substr(pos + 16, line.length()));
			size_t len;
			num >> len;
			std::cout << "Content-Length : " << len << std::endl;
			_contentLength = len;
			return len;
		}
	}
	return 0;
}

static std::string removeWhiteSpace(std::string str){
	std::stringstream ss;
	std::string removeline;
	ss.str(str);
	std::getline(ss, removeline);
	ss >> removeline >> std::ws;
	removeline.find("\r") != std::string::npos ? removeline.erase(removeline.find("\r")) : removeline;
	return removeline;
}

std::string CgiPostHandle::setContentType(std::string line){
	std::string type = "";
	size_t pos = line.find("Content-Type:");
	if (pos != std::string::npos){
		type = line.substr(pos + 14, line.length());
		type = removeWhiteSpace(type);
	}
	std::cout << "Content type : " << type << std::endl;
	return type;
}

void CgiPostHandle::setBoundary(std::string boundary){
	_boundary = removeWhiteSpace(boundary);
	// _boundary.insert(0, "-");
	std::cout << "_boundary : " << _boundary << std::endl;
}

std::string CgiPostHandle::findBoundary(std::string line){
	std::string boundary = "";
	// if (line.find("multipart/form-data") != std::string::npos){
		// size_t pos = line.find("boundary=");
		// if (pos != std::string::npos){
			std::cout << "line : " << line << std::endl;
			boundary = removeWhiteSpace(line);
			// boundary.insert(0, "--");
			std::cout << "boundary : " << std::endl;
		// }
	// }
	return boundary;
}

std::string CgiPostHandle::setFilename(std::string response){
	std::string filename = "";
	std::stringstream ss;
	ss.str(response);
	std::getline(ss, filename);
	ss >> filename >> std::ws;
	size_t pos = std::string::npos;
	if (filename.find("filename=") != std::string::npos) {
		pos = filename.find("filename=");	
		filename = response.substr(pos + 9, response.length());
	}
	else if (filename.find("name=") != std::string::npos){
		pos = filename.find("=");
		filename = response.substr(pos + 1, response.length());
	}
	else
		return "";
	ss.clear();
	ss.str(filename);
	std::getline(ss >> std::ws, filename);
	std::cout << "filename : " << filename.substr(1, filename.length() - 3) << std::endl;
	std::string path = "./html/download/";
	return path.append(filename.substr(1, filename.length() - 3));
}

size_t CgiPostHandle::findBytesWrite(std::string temp){
	std::string line;
	size_t pos;
	size_t size = 0;
	std::istringstream ss(temp);
	while (std::getline(ss, line)) {
		pos = line.find(_boundary);
		if (pos != std::string::npos){
			size += pos - 2;
			return size;
		}
		size += line.length() + 1;
	}
	return std::string::npos;
}
//----------------------------010738734963757867551732
//----------------------------010738734963757867551732--

size_t CgiPostHandle::findBytesStart(char *buffer, size_t len){
	std::string line;
	std::string temp = "";
	temp.append(buffer, len);
	std::istringstream ss(temp);
	size_t start = 0;
	while(std::getline(ss, line)){
		start += line.length() + 1;
		if (line.find("Content-Type:") != std::string::npos){
			_contentType = setContentType(line);
			if (_boundary.empty()){
				std::cout << "??\n";
				_boundary = findBoundary(_contentType);
			}
		}
		if (line.find("Content-Disposition:") != std::string::npos){
			size_t pos = line.find("filename=");
			if (pos != std::string::npos){
				_filename = setFilename(line);
				std::getline(ss, line);
				start += line.length() + 1;
				_contentType = setContentType(line);
				if (!_contentType.empty()){
					std::getline(ss, line);
					start += line.length() + 1;
					if (line[0] == '\r') {
						break ;
					}
				}
			}
		}
	}
	return start;
}

size_t CgiPostHandle::createFile(char *buffer, int sockfd, int bytes_received){
	// int bytes = 0;
	size_t start = findBytesStart(buffer, bytes_received);
	if (_filename.empty())
		return 0;
	std::ofstream outfile(_filename.c_str(), std::ios::out | std::ios::binary);
	std::string temp = "";
	size_t size = 0;
	if (bytes_received - start > 0){
		temp.append(&buffer[start], bytes_received - start);
		size = findBytesWrite(temp);
		if (size != std::string::npos) {
			outfile.write(&buffer[start], size);
			bytes_received = size;
		}
		else
			outfile.write(&buffer[start], bytes_received - start);
		_bytesRead += bytes_received;
		_bytesWrite += bytes_received;
		temp.clear();
	}
	std::cout << "bytes_received: " << bytes_received << std::endl;
	while (bytes_received >= 0){
		// if ((bytes = recv(sockfd, buffer, BUFFER_SIZE, MSG_PEEK) < 0)){
		// if ((bytes = read(sockfd, buffer, BUFFER_SIZE) < 0)){
		// 	if (errno == EAGAIN) {
		// 		continue;
		// 	}
		// 	else if (errno)
		// 		perror("read error : ");
		// }
		// bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
		bytes_received = read(sockfd, buffer, BUFFER_SIZE);
		std::cout << "bytes_received: " << bytes_received << std::endl;
		if (bytes_received > 0) {
			_bytesRead += bytes_received;
			temp.append(buffer, bytes_received);
			size = findBytesWrite(temp);
			temp.clear();
			
			std::cout << "size  write : " << size << std::endl;
			if (size != std::string::npos){
				std::cout << "buf write left: " << &buffer[size] << std::endl;
				outfile.write(buffer, size);
				_bytesWrite += size;
				break ;
			}
			outfile.write(buffer, bytes_received);
			_bytesWrite += bytes_received;
		}
		else 
			break;
	}
	temp.clear();
	outfile.close();
	// int se = 0;
	// while (true) {
	// 	if (bytes_received >= 0){
	// 		_response = "HTTP/1.1 200 OK\r\n"
	// 			"\r\n"
	// 			"<html><body>";
	// 		_response.append(_filename.append("received</body></html>\r\n"));
	// 		se = send(sockfd, _response.c_str(), _response.length(), 0);
	// 		std::cout << "send status : " << se << std::endl;
	// 		if (errno)
	// 			perror("send error : ");
	// 		_responseLength += _response.length();
	// 	}
	// 	else {
	// 		_response = "HTTP/1.1 405 Method Not Allowed\r\n"
	// 			"Connection: close\r\n"
	// 			"\r\n"
	// 			"<html><body>";
	// 		_response.append(_filename.append(" can not received</body></html>\r\n"));
	// 		se = send(sockfd, _response.c_str(), _response.length(), 0);
	// 		_responseLength += _response.length();
	// 	}
	// 	if (se > 0)
	// 		break ;
	// }
	_filename.erase(0, _filename.length());
	_filename.clear();
	return bytes_received;
}

void CgiPostHandle::setSockfd(int fd1, int fd2){
	_readfd = fd1;
	_writefd = fd2;
}

int CgiPostHandle::getReadfd(void){
	return _readfd;
}

int CgiPostHandle::getWritefd(void){
	return _writefd;

}

void CgiPostHandle::setFilebuffer(char *buffer){
	_filebuffer.push_back(buffer);
}

char *CgiPostHandle::getlastestFilebuffer(void){
	return _filebuffer.back();
}

size_t CgiPostHandle::getContentLength(void){
	return _contentLength;
}

size_t CgiPostHandle::getBytesWrite(void){
	return _bytesWrite;
}

size_t CgiPostHandle::getBytesRead(void){
	return _bytesRead;
}

std::string CgiPostHandle::getFilename(void){
	return _filename;
}

size_t CgiPostHandle::getResponseLength(void){
	return _responseLength;
}

std::string CgiPostHandle::getResponse(const char *msg){
	_responseLength += strlen(msg);
	std::string rs(msg);
	std::ostringstream oss;
	oss << _responseLength;
	oss.str().append("\r\n");
	rs.append(oss.str());
	return rs;
}

//findBytesstart
//createfile

		// std::ostringstream num, writefd;
		// num << sockfd;
		// std::cout << "sockd fd : " << sockfd << std::endl;
		// char *sock = new char[num.str().length() + 1];
		// strcpy(sock,const_cast<char *>(num.str().c_str()));
		// char cmd[18] = "./cgi-bin/testcgi";
		// char *arg[4] = {cmd, sock, sock, NULL};
		// int pid = fork();
		// if (pid == 0) {
		// 	if (execve(arg[0], &arg[0], NULL) == -1){
		// 		perror("");
		// 		std::cout << "execve failed\n";
		// 	}
		// }
		// waitpid(pid, NULL, 0);
		// std::cout << "parent received : " << buf << "\n";
		// std::string child(buf);
		// delete[] sock;