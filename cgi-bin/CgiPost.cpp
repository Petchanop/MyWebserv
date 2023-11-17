/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPost.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: npiya-is <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/11/08 14:20:49 by npiya-is          #+#    #+#             */
/*   Updated: 2023/11/09 20:27:08 by npiya-is         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../cgi/CgiPostHandle.hpp"

int main(int argc, char *argv[]){
	if (argc != 3) {
		std::cout << "Usage: ./CgiPost <char *>" << std::endl;
		return EXIT_FAILURE;
	}

	CgiPostHandle cgiPost;
	std::string temp = "";
	size_t bytes_write = 0;
	int writeStatus = 0;
	std::cout << "-------------Execute CgiPost-------------\n";
	std::cout << "sockfd: " << argv[2] << std::endl;
	temp.append(argv[1], strlen(argv[1]));
	bytes_write = cgiPost.findBytesWrite(temp);
	if (bytes_write != std::string::npos)
		writeStatus = write(std::atoi(argv[2]), argv[1], bytes_write);
	else
		writeStatus = write(std::atoi(argv[2]), argv[1], strlen(argv[2]));
	if (writeStatus < 0)
		perror("write error:");
	// std::cout << "write to file : " << temp.substr(bytes_write) << std::endl;
	return EXIT_SUCCESS;
}

