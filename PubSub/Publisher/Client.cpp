#include "Publisher.h"

DWORD WINAPI PublisherSend(LPVOID lpParam) {
	SOCKET connectSocket = *(SOCKET*)lpParam;

	while (appRunning && !serverStopped) {

		PrintMenu();
		char c = _getch();

		char message[270];

		if (c == '1' || c == '2' || c == '3' || c == '4' || c == '5') {

			ProcessInput(c, message);

			char publish_message[250];

			EnterAndGenerateMessage(publish_message, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStructToSend = GenerateMessageStruct(message, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStructToSend, messageSize);
			//free(messageStructToSend);
			if (sendResult == -1)
				break;

			
		}
		else if (c == 'x' || c == 'X') {
			char shutDownMessage[] = "p:shutDown";
			int messageDataSize = strlen(shutDownMessage) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(shutDownMessage, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStruct, messageSize);
			//free(messageStruct);
			if (sendResult == -1) {
				break;
			}

			appRunning = false;
			closesocket(connectSocket);
			//free(messageStructToSend);
			break;
		}
		else {
			printf("Invalid input.\n");
			continue;
		}
	}
	return 1;
}

DWORD WINAPI PublisherReceive(LPVOID lpParam) {
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	char recvbuf[DEFAULT_BUFLEN];
	char* recvRes = (char*)malloc(DEFAULT_BUFLEN);

	while (appRunning && !serverStopped)
	{
		recvRes = ReceiveFunction(connectSocket, recvbuf);
		//memcpy(recvbuf, ReceiveFunction(connectSocket, recvbuf), DEFAULT_BUFLEN);
		 if (!strcmp(recvRes, "ErrorS")) {

			/*free(recvRes);
			return 1;*/
			 break;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
			// connection was closed gracefully
			printf("\nConnection with server closed.\n");
			printf("Press any key to close this window . . .");
			closesocket(connectSocket);
			appRunning = false;
			serverStopped = true;
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			appRunning = false;
			serverStopped = true;
			/*free(recvRes);
			return 1;*/
			break;
		}
	}
	//free(recvRes);

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


	char connect[] = "p:Connect";

	int messageDataSize = strlen(connect) + 1;
	int messageSize = messageDataSize + sizeof(int);

	MessageStruct* messageStruct = GenerateMessageStruct(connect, messageDataSize);
	int sendResult = SendFunction(connectSocket, (char*)messageStruct, messageSize);
	if (sendResult == -1)
		serverStopped = true;


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

	free(messageStruct);

	WSACleanup();

	return 0;
}

