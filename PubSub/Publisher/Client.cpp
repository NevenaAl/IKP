#include "Publisher.h"


///<summary>
/// A function executing in thread created and run at the begining of main program. 
/// It is used for sending messages to Server.
///</summary>
///<param name ="lpParam"> Socket used to communicate with Server. </param>
///<returns>No return value.</returns>
DWORD WINAPI PublisherSend(LPVOID lpParam) {
	SOCKET connectSocket = *(SOCKET*)lpParam;

	while (appRunning && !serverStopped) {

		PrintMenu();
		char c = _getch();

		char* message = (char*)malloc(270 * sizeof(char));

		if (c == '1' || c == '2' || c == '3' || c == '4' || c == '5') {

			ProcessInput(c, message);

			char* publish_message = (char*)malloc(250*sizeof(char));

			EnterAndGenerateMessage(publish_message, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStructToSend = GenerateMessageStruct(message, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStructToSend, messageSize);
			free(messageStructToSend);
			free(message);
			free(publish_message);
			if (sendResult == -1)
				break;

			
		}
		else if (c == 'x' || c == 'X') {
			strcpy(message, "p:shutDown");
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
/// It is used for receiving shutdown message from Server.
///</summary>
///<param name ="lpParam"> Socket used to communicate with Server. </param>
///<returns>No return value.</returns>
DWORD WINAPI PublisherReceive(LPVOID lpParam) {
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	char recvbuf[DEFAULT_BUFLEN];

	while (appRunning && !serverStopped)
	{
		char* recvRes;

		recvRes = ReceiveFunction(connectSocket, recvbuf);
		//memcpy(recvbuf, ReceiveFunction(connectSocket, recvbuf), DEFAULT_BUFLEN);
	    if (!strcmp(recvRes, "ErrorS")) {
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
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			appRunning = false;
			serverStopped = true;
			free(recvRes);
			break;
		}
		free(recvRes);
		
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

	connectSocket = socket(AF_INET,	SOCK_STREAM, IPPROTO_TCP);

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
		serverStopped = true;
	}
		
	HANDLE publisherSendThread, publisherReceiveThread;
	DWORD publisherSendID, publisherReceiveID;

	publisherSendThread = CreateThread(NULL, 0, &PublisherSend, &connectSocket, 0, &publisherSendID);
	publisherReceiveThread = CreateThread(NULL, 0, &PublisherReceive, &connectSocket, 0, &publisherReceiveID);

	//PUBLISHER WHILE
	while (appRunning && !serverStopped) {

	}


	if (publisherSendThread)
		WaitForSingleObject(publisherSendThread, INFINITE);
	if (publisherReceiveThread)
		WaitForSingleObject(publisherReceiveThread, INFINITE);

	SAFE_DELETE_HANDLE(publisherSendThread);
	SAFE_DELETE_HANDLE(publisherReceiveThread);
	
	closesocket(connectSocket);

	WSACleanup();

	return 0;
}

