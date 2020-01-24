#include "Publisher.h"

int possible = 1;

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
	SendFunction(connectSocket, (char*)messageStruct, messageSize);

	//PUBLISHER WHILE
	while (possible) {

		PrintMenu();
		char c = _getch();

		char message[120];

		if (c == '1' || c=='2' || c=='3' || c=='4' || c=='5') {
			
			ProcessInput(c, message);

			char publish_message[100];

			EnterAndGenerateMessage(publish_message, message);

			int messageDataSize = strlen(message) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
			SendFunction(connectSocket, (char*)messageStruct, messageSize);

		}
		else if (c == 'x' || c == 'X') {
			char shutDownMessage[20] = "p:shutDown";
			int messageDataSize = strlen(shutDownMessage) + 1;
			int messageSize = messageDataSize + sizeof(int);

			MessageStruct* messageStruct = GenerateMessageStruct(shutDownMessage, messageDataSize);
			SendFunction(connectSocket, (char*)messageStruct, messageSize);
			possible = 0;
			closesocket(connectSocket);
			break;
		}
		else {
			printf("Invalid input.\n");
			continue;
		}
	}
	
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

