#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")


int sError = -1;
int bytes_received = -1;
SOCKET cli_socket;
const unsigned int BUF_LEN = 512;

char recv_buf[BUF_LEN];

void sending()
{
	for (;;) {
		std::string line;
		std::getline(std::cin, line);
		if (line == "/exit") {
			line = ">>Disconnecting>>";
			char* message = (char*)line.c_str();
			send(cli_socket, message, strlen(message), 0);
			break;
		}
		else if (line == "/cmd")
		{
			std::cout << "Join a Room: \t/join:(Room#)\n";
			std::cout << "Change Name: \t/name:(Name)\n";
			std::cout << "Display Lobby:\t /online:(Name)\n";
			std::cout << "Disconnect: \t/exit\n";
		}
		else
		{
			char* message = (char*)line.c_str();
			send(cli_socket, message, strlen(message), 0);
		}
	}
}

void receiving()
{
	for (;;) {
		memset(recv_buf, 0, BUF_LEN);

		bytes_received = recv(cli_socket, recv_buf, BUF_LEN, 0);
		sError = WSAGetLastError();
		if (sError != WSAEWOULDBLOCK && bytes_received > 0)
		{
			printf(recv_buf);
			memset(recv_buf, 0, BUF_LEN);
		}
	}
}


int main() {

	//Initialize winsock
	WSADATA wsa;

	//Get IP address
	std::string getIpAddress;
	std::cout << "Enter IP Adress>> ";
	std::getline(std::cin, getIpAddress);
	const char* ipAddress = getIpAddress.c_str();



	int error;
	error = WSAStartup(MAKEWORD(2, 2), &wsa);

	if (error != 0) {
		printf("Failed to initialize %d\n", error);
		return 1;
	}

	//Create a client socket

	struct addrinfo* ptr = NULL, hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;



	if (getaddrinfo(ipAddress, "5000", &hints, &ptr) != 0) {
		printf("Getaddrinfo failed!! %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	cli_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (cli_socket == INVALID_SOCKET) {
		printf("Failed creating a socket %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}



	//Connect to the server
	if (connect(cli_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
		printf("Unable to connect to server: %d\n", WSAGetLastError());
		closesocket(cli_socket);
		freeaddrinfo(ptr);
		WSACleanup();
		return 1;
	}

	printf("Connected to the server\n");

	const unsigned int BUF_LEN = 512;

	char recv_buf[BUF_LEN];
	memset(recv_buf, 0, BUF_LEN);

	// Change to non-blocking mode
	u_long mode = 1; // 0 for blocking mode
	ioctlsocket(cli_socket, FIONBIO, &mode);

	std::thread send(sending);
	std::thread receive(receiving);

	receive.detach();
	send.join();


	//Shutdown the socket

	if (shutdown(cli_socket, SD_BOTH) == SOCKET_ERROR) {
		printf("Shutdown failed!  %d\n", WSAGetLastError());
		closesocket(cli_socket);
		WSACleanup();
		return 1;
	}

	closesocket(cli_socket);
	freeaddrinfo(ptr);
	WSACleanup();

	return 0;
}