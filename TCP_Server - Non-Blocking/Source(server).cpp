#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>



#pragma comment(lib, "Ws2_32.lib")
SOCKET _socket;
fd_set master_sockets;
SOCKET server_socket;
SOCKET out_socket;
const unsigned int BUF_LEN = 512;
char buf[4096];


struct Client {
	Client(SOCKET _sock, std::string _name, int _room) : sock(_sock), name(_name), room(_room) {};
	SOCKET sock;
	std::string name;
	int room;
};

std::vector<Client> clients;
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
				std::string welcomeMsg = "SERVER>> Welcome to the Cerber(US)!!!\tUse /cmd to see commands\r\n";

				if (send(client_socket, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0) == SOCKET_ERROR) {
					printf("Failed to send msg to client %d\n", WSAGetLastError());
					closesocket(client_socket);
					freeaddrinfo(ptr);
					WSACleanup();
					return 1;
				}

				Client tempClient(client_socket, "User" , 0); //Create struct of joining client
				clients.push_back(tempClient);

				for (int j = 0; j < clients.size(); j++)
				{
					out_socket = clients[j].sock;
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
				else if (buf[0] == '/')
				{
					std::cout << "CommandRecognized\n";
					std::string tmp = buf;
					std::size_t pos = tmp.find(":");					//Find First Break
					tmp = tmp.substr(1, pos - 1);
					if (tmp == "join")
					{
						std::cout << "JoinRoomCalled\n";
						tmp = buf;
						tmp = tmp.substr(pos + 1);
						for (int j = 0; j < clients.size(); j++)
						{
							if (_socket == clients[j].sock)
							{
								clients[j].room = std::stof(tmp, NULL);
								std::cout << "Changed room of User: " << clients[j].sock << " to " << "room " << clients[j].room << "\n";
							}
						}
					}
					else if (tmp == "name")
					{
						std::cout << "NameChangeCalled\n";
						tmp = buf;
						tmp = tmp.substr(pos + 1);
						for (int j = 0; j < clients.size(); j++)
						{
							if (_socket == clients[j].sock)
							{
								clients[j].name = tmp;
								std::cout << "Changed name of User: " << clients[j].sock << " to " << "'" << clients[j].name << "'\n";
							}
						}

					}
				}
				else
				{
					//Send to everyone in room
					for (int j = 0; j < clients.size(); j++)
					{
						if (clients[j].sock == _socket) //Check who is sending
						{
							for (int k = 0; k < clients.size(); k++)
							{
								if (clients[k].sock != server_socket && clients[k].sock != _socket && clients[k].room == clients[j].room) //If not the client sending, check to see if in same room
								{
									std::string string_Out;
									if (clients[j].name != "User")
									{
										string_Out = clients[j].name + ">> " + buf + "\r\n";;
									}
									else
									{
										std::ostringstream ss;
										ss << "User: " << clients[j].sock << ">> " << buf << "\r\n";
										string_Out = ss.str();
									}
									send(clients[k].sock, string_Out.c_str(), string_Out.size() + 1, 0);
								}
							}
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