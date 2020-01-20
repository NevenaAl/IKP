
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016
#define SERVER_SLEEP_TIME 50

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.
bool InitializeWindowsSockets();
void SelectFunc(int, SOCKET, char);
void PrintMenu();
void ProcessInputAndGenerateMessage(char input, char* message);

int __cdecl main(int argc, char **argv)
{
	// socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;
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

	char ch = _getch();

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}
void SelectFunc(int iResult, SOCKET listenSocket, char rw) {
	do {
		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);
		// Add socket we will wait to read from
		FD_SET(listenSocket, &set);

		// Set timeouts to zero since we want select to return
		// instantaneously
		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (rw == 'r') {
			iResult = select(0 /* ignored */, &set, NULL, NULL, &timeVal);
		}
		else {
			iResult = select(0 /* ignored */, NULL, &set, NULL, &timeVal);
		}


		// lets check if there was an error during select
		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		// now, lets check if there are any sockets ready
		if (iResult == 0)
		{
			// there are no ready sockets, sleep for a while and check again
			Sleep(SERVER_SLEEP_TIME);
			continue;
		}
		break;
		//NEW
	} while (1);

}

void PrintMenu() {
	printf("\nChoose a topic to subscribe to: \n");
	printf("\t1.Sport\n");
	printf("\t2.Fashion\n");
	printf("\t3.Politics\n");
	printf("\t4.News \n");
	printf("\t5.Show buisness \n\n");
	printf("Press X if you want to close connection\n");
}

void ProcessInputAndGenerateMessage(char input, char* message) {
	switch (input) {
	case '1':
		strcpy(message, "s:Sport");
		break;
	case '2':
		strcpy(message, "s:Fashion");
		break;
	case '3':
		strcpy(message, "s:Politics");
		break;
	case '4':
		strcpy(message, "s:News");
		break;
	case '5':
		strcpy(message, "s:Show business");
		break;
	default:
		break;
	}
}