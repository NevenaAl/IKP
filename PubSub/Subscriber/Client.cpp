#include "Subscriber.h"

struct topic_message {
	char topic[15];
	char message[250];
};

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.

int __cdecl main(int argc, char **argv)
{
	// socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;
	char recvbuf[DEFAULT_BUFLEN];
	// variable used to store function return value
	int iResult;

	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	// create a socket
	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// create and initialize address structure
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(DEFAULT_PORT);
	// connect to server specified in serverAddress and socket connectSocket
	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);

	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	while (true) {

		PrintMenu();
		
		char c = _getch();

		SelectFunc(iResult, connectSocket, 'w');

		char message[20];

		if (c == '1' || c == '2' || c == '3' || c == '4' || c == '5') {

			ProcessInputAndGenerateMessage(c, message);

			iResult = send(connectSocket, (char*)(&message), sizeof(message), 0);
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}

			printf("Bytes Sent: %ld\n", iResult);
			break;
		}
		else if (c == 'x' || c == 'X') {
			closesocket(connectSocket);
			break;
		}
		else {
			printf("Invalid input.\n");
			continue;
		}
		// Send an prepared message with null terminator included
	}
	
	//printf("Bytes Sent: %ld\n", iResult);

	while (true) {
		SelectFunc(iResult, connectSocket, 'r');

		iResult = recv(connectSocket, recvbuf, sizeof(topic_message), 0);
		topic_message* structrecv = (topic_message*)malloc(sizeof(topic_message));
		topic_message msg = *(topic_message*)recvbuf;

		strcpy(structrecv->message, msg.message);
		strcpy(structrecv->topic, msg.topic);

		if (iResult > 0)
		{
			printf("\nNew message: %s on topic: %s\n", structrecv->message, structrecv->topic);
		}
		else if (iResult == 0)
		{
			// connection was closed gracefully
			printf("Connection with server closed.\n");
			closesocket(connectSocket);
		}
		else
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
		}
	}

	char ch = _getch();

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

