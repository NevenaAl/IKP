#include "Publisher.h"

bool appRunning = true;

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

	//PUBLISHER WHILE
	while (appRunning && !serverStopped) {

		PrintMenu();
		char c = _getch();

		char message[270];

		if (c == '1' || c=='2' || c=='3' || c=='4' || c=='5') {
			
			ProcessInput(c, message);

			char publish_message[250];

			EnterAndGenerateMessage(publish_message, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStructToSend = GenerateMessageStruct(message, messageDataSize);
			int sendResult = SendFunction(connectSocket, (char*)messageStructToSend, messageSize);
			if (sendResult == -1)
				break;

			free(messageStructToSend);
		}
		else if (c == 'x' || c == 'X') {
			char shutDownMessage[] = "p:shutDown";
			int messageDataSize = strlen(shutDownMessage) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStructToSend = GenerateMessageStruct(shutDownMessage, messageDataSize);
			SendFunction(connectSocket, (char*)messageStructToSend, messageSize);
			appRunning = false;
			closesocket(connectSocket);
			free(messageStructToSend);
			break;
		}
		else {
			printf("Invalid input.\n");
			continue;
		}
	}
	
	closesocket(connectSocket);

	free(messageStruct);

	WSACleanup();

	return 0;
}

