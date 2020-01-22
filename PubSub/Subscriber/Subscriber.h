#pragma once
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



bool InitializeWindowsSockets();
void SelectFunction(SOCKET, char);
void PrintMenu();
void ProcessInputAndGenerateMessage(char input, char* message);
void SendFunction(SOCKET, char*, int);
void ReceiveFunction(SOCKET acceptedSocket, char* recvbuf);
char* GenerateMessage(char* message, int len);

char* GenerateMessage(char* message, int len) {

	char* messageToSend = (char*)malloc(len + sizeof(int));
	int* headerPointer = (int*)messageToSend;
	*headerPointer = len;
	++headerPointer;
	char* messageValue = (char*)headerPointer;

	memcpy(messageValue, message, len);

	return messageToSend;
}

void ReceiveFunction(SOCKET acceptedSocket, char* recvbuf) {

	int iResult;

	do {

		// Receive data until the client shuts down the connection
		SelectFunction(acceptedSocket,'r');
		iResult = recv(acceptedSocket, recvbuf, 4, 0); // primamo samo header poruke

		if (iResult > 0)
		{
			int bytesExpected = *((int*)recvbuf);
			printf("Size of message is : %d\n", bytesExpected);

			char* myBuffer = (char*)(malloc(sizeof(bytesExpected))); // alociranje memorije za poruku

			int cnt = 0;

			while (cnt < bytesExpected) {

				SelectFunction(acceptedSocket,'r');
				iResult = recv(acceptedSocket, myBuffer + cnt, bytesExpected - cnt, 0);

				printf("Message received from client: %s.\n", myBuffer);

				cnt += iResult;
			}

		}
		else if (iResult == 0)
		{
			// connection was closed gracefully
			printf("Connection with client closed.\n");
			closesocket(acceptedSocket);
		}
		else
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
		}


	} while (iResult > 0);

}

void SendFunction(SOCKET connectSocket, char* message, int messageSize) {

	SelectFunction(connectSocket, 'w');
	int iResult = send(connectSocket, message, messageSize, 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return;
	}
	else {

		int cnt = iResult;
		while (cnt < messageSize) {

			SelectFunction(connectSocket, 'w');
			iResult = send(connectSocket, message + cnt, messageSize - cnt, 0);
			cnt += iResult;
		}
	}

	//printf("Bytes Sent: %ld\n", iResult);
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
void SelectFunction(SOCKET listenSocket, char rw) {
	int iResult = 0;
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