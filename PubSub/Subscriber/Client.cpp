#include "Subscriber.h"


HANDLE SubscriberSendThread, SubscriberRecvThread;
DWORD SubscriberSendThreadId, SubscriberRecvThreadId;

///<summary>
/// A function executing in thread created and run at the begining of main program. 
/// It is used for sending messages to Server.
///</summary>
///<param name ="lpParam"> Socket used to communicate with Server. </param>
///<returns>No return value.</returns>
DWORD WINAPI SubscriberSend(LPVOID lpParam) {
	int subscribedTopics[5];
	int numOfSubscribedTopics = 0;
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	while (appRunning && !serverStopped) {

		PrintMenu();

		char input = _getch();
		//char input = '1';

		char* message = (char*)malloc(20 * sizeof(char));

		if (input == '1' || input == '2' || input == '3' || input == '4' || input == '5') {

			if (AlreadySubscribed(input, subscribedTopics, numOfSubscribedTopics)) {
				printf("You are already subscribed to this topic.\n");
				continue;
			}

			subscribedTopics[numOfSubscribedTopics] = input - '0';
			numOfSubscribedTopics++;

			ProcessInputAndGenerateMessage(input, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStruct, messageSize);
			free(messageStruct);
			free(message);
			if (sendResult == -1) {
				return 1;
			}

		}
		else if (input == 'x' || input == 'X') {
			strcpy(message, "s:shutDown");
			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);
				
			MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStruct, messageSize);
			free(messageStruct);
			free(message);
			if (sendResult == -1) {
				break;
			}

			appRunning = false;
			closesocket(connectSocket);
			break;
		}
		else {
			printf("Invalid input.\n");
			free(message);
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
	char* recvRes; 

	while (appRunning && !serverStopped)
	{
		recvRes = ReceiveFunction(connectSocket, recvbuf);
		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			char delimiter[] = ":";

			char *ptr = strtok(recvRes, delimiter);

			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			char *message = ptr;
			ptr = strtok(NULL, delimiter);

			printf("\nNew message: %s on topic: %s\n", message, topic);

			free(recvRes);
		}
		else if(!strcmp(recvRes, "ErrorS")) {
			closesocket(connectSocket);
			appRunning = false;
			serverStopped = true;
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
			printf("\nConnection with server closed.\n");
			printf("Press any key to close this window . . .");
			closesocket(connectSocket);
			appRunning = false;
			serverStopped = true;
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			appRunning = false;
			serverStopped = true;
			free(recvRes);
			break;
		}

	}
	return 1;
}
int __cdecl main(int argc, char **argv)
{
	SOCKET connectSocket = INVALID_SOCKET;
	
	int iResult;

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(DEFAULT_PORT);

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

	int connectResult = Connect(connectSocket);
	if (connectResult == -1) {
		appRunning = false;
		serverStopped = true;
	}
	

	SubscriberSendThread = CreateThread(NULL, 0, &SubscriberSend, &connectSocket, 0, &SubscriberSendThreadId);
	SubscriberRecvThread = CreateThread(NULL, 0, &SubscriberReceive, &connectSocket, 0, &SubscriberRecvThreadId);


	while (appRunning || !serverStopped) {

	}

	if (SubscriberSendThread)
		WaitForSingleObject(SubscriberSendThread, INFINITE);
	if (SubscriberRecvThread)
		WaitForSingleObject(SubscriberRecvThread, INFINITE);

	SAFE_DELETE_HANDLE(SubscriberSendThread);
	SAFE_DELETE_HANDLE(SubscriberRecvThread);

	closesocket(connectSocket);


	WSACleanup();

	return 0;
}

