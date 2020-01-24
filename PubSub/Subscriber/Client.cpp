#include "Subscriber.h"

#define SAFE_DELETE_HANDLE(h) {if(h)CloseHandle(h);}

struct topic_message {
	char topic[15];
	char message[250];
};


int recvPossible = 1;
int sendPossible = 1;

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.
DWORD WINAPI SubscriberSend(LPVOID lpParam) {
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	while (sendPossible) {

		PrintMenu();

		char c = _getch();

		char message[20];

		if (c == '1' || c == '2' || c == '3' || c == '4' || c == '5') {

			ProcessInputAndGenerateMessage(c, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
			SendFunction(connectSocket, (char*)messageStruct, messageSize);

		}
		else if (c == 'x' || c == 'X') {
			char shutDownMessage[20] = "s:shutDown";
			int messageDataSize = strlen(shutDownMessage) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(shutDownMessage, messageDataSize);
			SendFunction(connectSocket, (char*)messageStruct, messageSize);

			sendPossible = 0;
			recvPossible = 0;
			closesocket(connectSocket);
			break;
		}
		else {
			printf("Invalid input.\n");
			continue;
		}
	}
	return 1;
}
DWORD WINAPI SubscriberReceive(LPVOID lpParam) {
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	char recvbuf[DEFAULT_BUFLEN];
	while (recvPossible) 
	{
		memcpy(recvbuf, ReceiveFunction(connectSocket, recvbuf), DEFAULT_BUFLEN);
		if (strcmp(recvbuf, "ErrorC") && strcmp(recvbuf, "ErrorR"))
		{
		  topic_message* structrecv = (topic_message*)malloc(sizeof(topic_message));
		  topic_message msg = *(topic_message*)recvbuf;

		  strcpy(structrecv->message, msg.message);
		  strcpy(structrecv->topic, msg.topic);

		  printf("\nNew message: %s on topic: %s\n", structrecv->message, structrecv->topic);
		}
		else if (!strcmp(recvbuf, "ErrorC"))
		{
		// connection was closed gracefully
			printf("Connection with server closed.\n");
			closesocket(connectSocket);
			break;
		}
		else if (!strcmp(recvbuf, "ErrorR"))
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			break;
		}
	}
	return 1;
}
int __cdecl main(int argc, char **argv)
{
	// socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;
	
	// variable used to store function return value
	int iResult;

	if (InitializeWindowsSockets() == false)
	{
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
	HANDLE SubscriberSendThread,SubscriberRecvThread;
	DWORD SubscriberSendThreadId, SubscriberRecvThreadId;

	char message[20] = "s:connect";
	int messageDataSize = strlen(message) + 1;
	int messageSize = messageDataSize + sizeof(int);

	MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
	SendFunction(connectSocket, (char*)messageStruct, messageSize);

	SubscriberSendThread = CreateThread(NULL, 0, &SubscriberSend, &connectSocket, 0, &SubscriberSendThreadId);
	SubscriberRecvThread = CreateThread(NULL, 0, &SubscriberReceive, &connectSocket, 0, &SubscriberRecvThreadId);


	while (recvPossible && sendPossible) {

	}

	if (SubscriberSendThread)
		WaitForSingleObject(SubscriberSendThread, INFINITE);
	if (SubscriberRecvThread)
		WaitForSingleObject(SubscriberRecvThread, INFINITE);

	SAFE_DELETE_HANDLE(SubscriberSendThread);
	SAFE_DELETE_HANDLE(SubscriberRecvThread);
	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

