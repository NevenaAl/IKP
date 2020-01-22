#include "Publisher.h"

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
	SelectFunction(connectSocket, 'w');
	iResult = send(connectSocket, (char*)(&connect), sizeof(connect), 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	//printf("Bytes Sent: %ld\n", iResult);
	//SendFunction(connectSocket, (char*)&connect, sizeof(connect));

	//PUBLISHER WHILE
	while (true) {

		PrintMenu();
		char c = _getch();

		char message[120];

		if (c == '1' || c=='2' || c=='3' || c=='4' || c=='5') {
			
			ProcessInput(c, message);

			char publish_message[100];

			EnterAndGenerateMessage(publish_message, message);

			SelectFunction(connectSocket, 'w');
			iResult = send(connectSocket, (char*)(&message), sizeof(message), 0);
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
			//char* messageWithHeader = GenerateMessage((char*)&message, strlen(message)+1);
			//SendFunction(connectSocket, messageWithHeader, strlen(message) + 1 +sizeof(int));

			//printf("Bytes Sent: %ld\n", iResult);
		}
		else if (c == 'x' || c == 'X') {
			closesocket(connectSocket);
			break;
		}
		else {
			printf("Invalid input.\n");
			continue;
		}
	}
	

	//printf("Bytes Sent: %ld\n", iResult);

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

