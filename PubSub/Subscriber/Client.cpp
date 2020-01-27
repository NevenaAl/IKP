#include "Subscriber.h"

///<summary>
/// A function executing in thread created and run at the begining of main program. 
/// It is used for sending messages to Server.
///</summary>
///<param name ="lpParam"> Socket used to communicate with Server. </param>
///<returns>No return value.</returns>
DWORD WINAPI SubscriberSend(LPVOID lpParam) {
	int subscribed[5];
	int numOfSubscribedTopics = 0;
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	while (sendPossible) {

		PrintMenu();

		char c = _getch();

		char message[20];

		if (c == '1' || c == '2' || c == '3' || c == '4' || c == '5') {

			if (AlreadySubscribed(c, subscribed, numOfSubscribedTopics)) {
				printf("You are already subscribed to this topic.\n");
				continue;
			}

			subscribed[numOfSubscribedTopics] = c - '0';
			numOfSubscribedTopics++;

			ProcessInputAndGenerateMessage(c, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStruct, messageSize);
			free(messageStruct);
			if (sendResult == -1) {
				return 1;
			}

		}
		else if (c == 'x' || c == 'X') {
			char shutDownMessage[] = "s:shutDown";
			int messageDataSize = strlen(shutDownMessage) + 1;
			int messageSize = messageDataSize + sizeof(int);
				
			MessageStruct* messageStruct = GenerateMessageStruct(shutDownMessage, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStruct, messageSize);
			free(messageStruct);
			if (sendResult == -1) {
				break;
			}

			sendPossible = false;
			recvPossible = false;
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

///<summary>
/// A function executing in thread created and run at the begining of main program. 
/// It is used for receiving messages from Server.
///</summary>
///<param name ="lpParam"> Socket used to communicate with Server. </param>
///<returns>No return value.</returns>
DWORD WINAPI SubscriberReceive(LPVOID lpParam) {
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	char recvbuf[DEFAULT_BUFLEN];
	char* recvRes = (char*)malloc(DEFAULT_BUFLEN);

	while (true) 
	{
		recvRes = ReceiveFunction(connectSocket, recvbuf);
		//memcpy(recvbuf, ReceiveFunction(connectSocket, recvbuf), DEFAULT_BUFLEN);
		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			char delimiter[] = ":";
			char *ptr = strtok(recvRes, delimiter);

			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			char *message = ptr;
			ptr = strtok(NULL, delimiter);

			printf("\nNew message: %s on topic: %s\n", message, topic);

		}
		else if(!strcmp(recvRes, "ErrorS")) {
			return 1;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
		// connection was closed gracefully
			printf("Connection with server closed.\n");
			closesocket(connectSocket);
			sendPossible = false;
			recvPossible = false;
			return 1;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			sendPossible = false;
			recvPossible = false;
			return 1;
		}
	}
	free(recvRes);
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

	char message[] = "s:connect";
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

	free(messageStruct);

	WSACleanup();

	return 0;
}

