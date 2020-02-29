#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>



#pragma comment(lib, "Ws2_32.lib")
SOCKET _socket;
fd_set master_sockets;
SOCKET server_socket;
SOCKET out_socket;
const unsigned int BUF_LEN = 512;
char buf[4096];

int main() {

	//Initialize winsock
	WSADATA wsa;


	int error;
	error = WSAStartup(MAKEWORD(2, 2), &wsa);

	if (error != 0) {
		printf("Failed to initialize %d\n", error);
		return 1;
	}

	//Create a Server socket
	struct addrinfo* ptr = NULL, hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(NULL, "5000", &hints, &ptr) != 0) {
		printf("Getaddrinfo failed!! %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_socket == INVALID_SOCKET) {
		printf("Failed creating a socket %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Bind socket
	if (bind(server_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
		printf("Bind failed: %d\n", WSAGetLastError());
		closesocket(server_socket);
		freeaddrinfo(ptr);
		WSACleanup();
		return 1;
	}

	// Listen on socket
	if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed: %d\n", WSAGetLastError());
		closesocket(server_socket);
		freeaddrinfo(ptr);
		WSACleanup();
		return 1;
	}

	printf("Waiting for connections...\n");

	FD_ZERO(&master_sockets);
	FD_SET(server_socket, &master_sockets);

	bool application_running = true;

	while (application_running) {

		fd_set copy = master_sockets;
		int socket_Count = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socket_Count; i++) {
			_socket = copy.fd_array[i];
			if (_socket == server_socket) {
				// Accept a connection (multiple clients --> threads)
				SOCKET client_socket = accept(server_socket, NULL, NULL);

				if (client_socket == INVALID_SOCKET) {
					printf("Accept() failed %d\n", WSAGetLastError());
					closesocket(server_socket);
					freeaddrinfo(ptr);
					WSACleanup();
					return 1;

				}

				FD_SET(client_socket, &master_sockets);
				printf("Client connected!!\n");

				//Send welcome message
				std::string welcomeMsg = "SERVER>> Welcome to the Cerber(US)!!!\r\n";

				if (send(client_socket, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0) == SOCKET_ERROR) {
					printf("Failed to send msg to client %d\n", WSAGetLastError());
					closesocket(client_socket);
					freeaddrinfo(ptr);
					WSACleanup();
					return 1;
				}

				for (int j = 0; j < master_sockets.fd_count; j++)
				{
					out_socket = master_sockets.fd_array[j];
					if (out_socket != server_socket && out_socket != _socket && out_socket != client_socket)
					{
						std::ostringstream msgOUT;
						msgOUT << "User " << client_socket << ": --ONLINE--" << buf << "\r\n";
						std::string string_out = msgOUT.str();

						if (send(out_socket, string_out.c_str(), string_out.size()+1, 0) == SOCKET_ERROR) {
							printf("Failed to send msg to client %d\n", WSAGetLastError());
							closesocket(client_socket);
							freeaddrinfo(ptr);
							WSACleanup();
							return 1;
						}
					}
				}
			}
			else //inbound message
			{
				ZeroMemory(buf, 4096);

				//Receive message
				int bytesIn = recv(_socket, buf, 4096, 0);

				if (bytesIn <= 0)
				{
					//Drop the client
					closesocket(_socket);
					FD_CLR(_socket, &master_sockets);
				}
				//else if (buf[0] == 's')
				//{
				//	std::cout << server_socket << std::endl;
				//}
				//else if (buf[0] == 'o')
				//{
				//	std::cout << out_socket << std::endl;
				//}
				//else if (buf[0] == 'o')
				//{
				//	std::cout << _socket << std::endl;
				//}
				else
				{
					////Check to see if it is a command \quit kills the server
					//if (buf[0] == '/')
					//{
					//	//Is the command quit?
					//	std::string cmd = std::string(buf, bytesIn);
					//	if (cmd == "/exit")
					//	{
					//		application_running = false;
					//		break;
					//	}
					//
					//	//unknown command
					//	continue;
					//}

					//Send message to other clients, and definiately NOT the listening socket
					for (int i = 0; i < master_sockets.fd_count; i++)
					{
						out_socket = master_sockets.fd_array[i];
						if (out_socket != server_socket && out_socket != _socket)
						{
							std::ostringstream ss;
							ss << "User " << _socket << ">> " << buf << "\r\n";
							std::string strOut = ss.str();
					
							send(out_socket, strOut.c_str(), strOut.size() + 1, 0);
						}
					}
				}
			}
		}
	}

	//Remove the listening socket from the master file descriptor set and close it to prevent anyone else trying to connect
	FD_CLR(server_socket, &master_sockets);
	closesocket(server_socket);

	//Message to let users know what is happening
	std::string msg = "Cerber(US) is Shutting Down.\r\n";

	while (master_sockets.fd_count > 0)
	{
		//Get the socket number
		SOCKET soc = master_sockets.fd_array[0];

		//Send peace out message
		send(soc, msg.c_str(), msg.size() + 1, 0);

		//Remove it from master file list and close socket
		FD_CLR(soc, &master_sockets);
		closesocket(soc);
	}

	//Cleanup winsock
	WSACleanup();

	return 0;
}