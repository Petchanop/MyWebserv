/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiFormHandle.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: npiya-is <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/29 18:43:52 by npiya-is          #+#    #+#             */
/*   Updated: 2023/11/15 03:23:24 by npiya-is         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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


//download file form method

// Simple HTTP response for successful file upload
const char *successResponse =
	"HTTP/1.1 200 OK\r\n"
	"Connection: close\r\n"
	"\r\n"
	"<html><body>File received successfully</body></html>\n"
	"Content-Type: text/html\r\n"
	"Content-Length: ";

// Simple HTTP response for invalid request
const char *invalidResponse =
	"HTTP/1.1 405 Method Not Allowed\r\n"
	"Connection: close\r\n"
	"\r\n"
	"<html><body>Method Not Allowed</body></html>\n"
	"Content-Length: ";

const int BUFFER_SIZE = 100000;

#include "../cgi/CgiPostHandle.hpp"

int main(int argc, char *argv[]){
	char buffer[BUFFER_SIZE + 1] = {0};
	std::string rs;
	std::cout << "-------------Execute FormCgi-------------\n";
	std::cout << "argc: " << argc << std::endl;
	if (argc != 3){
		std::cout << "Usage: ./cgiFormHandle socketfd filebuffer boundary contentlength\n";
		return EXIT_FAILURE;
	}
	int sockfd = std::atoi(argv[1]);
	int writefd = std::atoi(argv[2]);
	int bytes_received = 0;
	// bytes_received = recv(sockfd, buffer, BUFFER_SIZE, MSG_PEEK);
	bytes_received = read(sockfd, buffer, BUFFER_SIZE);
	if (errno)
		perror("read error : ");
	// std::cout << "bytes_received: " << bytes_received << std::endl;
	CgiPostHandle cgiPost;
	cgiPost.setSockfd(sockfd, writefd);
	// cgiPost.findContentlength(buffer);
	std::stringstream fline(buffer);
	std::string boundary;
	getline(fline, boundary);
	cgiPost.setBoundary(boundary);
	fline.clear();
	// if ((bytes_received = recv(sockfd, buffer, bytes_received, 0)) < 0){
	// 	perror("read error : ");
	// 	return EXIT_FAILURE;
	// }
	while (bytes_received > 0) {
		bytes_received = cgiPost.createFile(buffer, sockfd, bytes_received);
	}
	if (bytes_received == 0){
		rs = cgiPost.getResponse(successResponse);
	}
	else {
		rs = cgiPost.getResponse(invalidResponse);
	}
	// int result = 0;
	// while (true) {
	// 	result = send(sockfd, rs.c_str(), rs.length(), 0);
	// 	if (result > 0)
	// 		break ;
	// }
	// wfd << cgiPost.getBytesWrite();
	// write(writefd, wfd.str().c_str(), wfd.str().length());
	std::cout << "-------------End FormCgi-------------\n";
	return EXIT_SUCCESS;
}
//c++ -Wall -Werror -Wextra -std=c++98 CgiFormHandle.cpp -o testcgi