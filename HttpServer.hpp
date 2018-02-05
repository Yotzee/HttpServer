/*
MIT License

Copyright(c) 2018 Gregory Allen Yotz Jr.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _HTTP_SERVER_HPP_
#define _HTTP_SERVER_HPP_

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <thread>
#include <vector>

#ifdef __WIN32__
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define response_resquest_obj std::map<std::string, std::string>

void middleware(response_resquest_obj req, response_resquest_obj response)
{
	printf("here");
	std::stringstream wr;
	wr << "Content-Type: application/json; charset=utf-8";
	wr << "Content-Length: 17";
	wr << "Date: Thu, 25 Jan 2018 00:46:30 GMT";
	wr << "Connection: close";
	wr << "{\"hello\":\"world\"}";
	response["write"] = wr.str();
}

class HttpServer
{

      private:
	int _port;
	int _sock;
	struct sockaddr_in addr_in;
	std::map<std::string, void (*)(response_resquest_obj, response_resquest_obj)> funcs;

      public:
	HttpServer()
	{
		_port = -1;
		funcs["*\\*"] = &middleware;
	}

	void setPort(int port)
	{
		_port = port;
	}

	void addEndPoint(std::string endpoint, void (*func)(response_resquest_obj, response_resquest_obj))
	{
		funcs[endpoint] = func;
	}

	int operator()()
	{
		struct addrinfo hints, *server;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		char *portChar = new char[6];
		sprintf(portChar, "%d", _port);
		if (getaddrinfo(NULL, portChar, &hints, &server) == -1) {
			std::cout << "error getaddrinfo" << std::endl;
			return -1;
		}

		_sock = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
		if (_sock == -1) {
			std::cout << "socker error" << std::endl;
			return 1;
		}

		memset(&addr_in, 0, sizeof(sockaddr_in));

		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(_port);
		addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(_sock, server->ai_addr, server->ai_addrlen) == -1) {
			std::cout << "binding error" << std::endl;
			close(_sock);
		}

		if (listen(_sock, 10) == -1) {
			std::cout << "listen error" << std::endl;
			close(_sock);
		}
		for (;;) {

			struct sockaddr_storage client_addr;
			socklen_t addr_size = sizeof client_addr;
			int fd = accept(_sock, (struct sockaddr *)&client_addr, &addr_size);
			std::cout << "request recieved" << std::endl;

			if (fd == -1) {
				close(_sock);
				return -1;
			}
			char buff[1024];
			memcpy(&buff, "\0", sizeof(buff));
			std::stringstream ss;
			int i = 0;
			while (recv(fd, buff, sizeof(buff) - 1, 0) != 0) {
				ss << buff;
			}
			std::cout << ss.str() << std::endl;

			std::stringstream wr;
			wr << "HTTP/1.1 200 OK\n";

			for (std::map<std::string, void (*)(response_resquest_obj, response_resquest_obj)>::iterator it = funcs.begin(); it != funcs.end(); ++it) {
				response_resquest_obj request, response;
				funcs["*\\*"](request, response);
				if (response["write"].length() > 0) {
					wr << response["write"];
				}
			}

			std::string wrstr = wr.str();
			std::cout << wrstr;
			write(fd, wr.str().c_str(), wr.str().length());
			close(fd);
		}

		if (shutdown(_sock, SHUT_RDWR) == -1) {
			close(_sock);
		}
		return 0;
	}
};

#endif
