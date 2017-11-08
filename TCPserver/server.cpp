#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

int main() {
	WSADATA wsaData;

	int iResult;

	//initialise Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

#define DEFAULT_PORT "80"

	//set ud a struct containing details about the socket, protocols etc. 
	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//resolve the local address and port to be used by the server, and save them for later in result
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//create the socket and set it's initial value as invalid, it's to be changed the the very next line anyway
	SOCKET ListenSocket = INVALID_SOCKET;
	// set up the listen socket details using result from earlier 
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Bind the TCP listening socket giving it our freshly created listenSocket and an adress 
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// free memory 
	freeaddrinfo(result);

	// keep listening till someone tries to connoct to the socket 
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//temporary socket
	SOCKET ClientSocket = INVALID_SOCKET;

	// Accept the client and give it the socket we just made 
	ClientSocket = accept(ListenSocket, NULL, NULL);  // null pointers because we dont care about what adress connects 
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//set up some buffers 
#define DEFAULT_BUFLEN 1024

	char recvbuf[DEFAULT_BUFLEN];
	char sendbuf[DEFAULT_BUFLEN];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;
	int sendbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {
		printf("im listening");

		//waiting for something to receive. 
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			
			
			// look if the carrier ID is even or odd  and send back a waiting time accordingly
			if (std::stoi(recvbuf) % 2 == 1) {
				char data[] = "10"; // odd numbered carriers wait 10 sec 
				strncpy_s(sendbuf, data, strlen(data)); // put into buffer 
			}
			else {
				char data[] = "5"; // even numbered carriers wait 5 sec
				strncpy_s(sendbuf, data, strlen(data)); // put into buffer 
			}

			// send the buffer 
			iSendResult = send(ClientSocket, sendbuf, strlen(sendbuf), 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		// if the client closed the socket we do so too 
		else if (iResult == 0) 
			printf("Connection closing...\n");
		// we received something unknown so we abort. 
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0); // while we receive something sensible. 

	// shutdown the server half of the connection since no more data will be sent
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}