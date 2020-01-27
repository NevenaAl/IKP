#pragma once
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "..\Common\SocketOperations.h"


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016
#define SERVER_SLEEP_TIME 50


bool recvPossible = true;
bool sendPossible = true;

int SelectFunction(SOCKET, char);
void PrintMenu();
void ProcessInputAndGenerateMessage(char input, char* message);
int SendFunction(SOCKET, char*, int);
char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf);
bool AlreadySubscribed(char, int[], int);

bool AlreadySubscribed(char c, int subscribed[], int numOfSubscribedTopics) {
	for (int i = 0; i < numOfSubscribedTopics; i++) {
		if (subscribed[i] == c - '0') {
			return true;
		}
	}
	return false;
}


char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf) {

	int iResult;

	int selectResult = SelectFunction(acceptedSocket, 'r');
	/*if (selectResult == -1) {
		return "ErrorS";
	}*/
	iResult = recv(acceptedSocket, recvbuf, 4, 0); // primamo samo header poruke

	if (iResult > 0)
	{
		int bytesExpected = *((int*)recvbuf);
		//printf("Size of message is : %d\n", bytesExpected);

		char* myBuffer = (char*)(malloc(sizeof(bytesExpected))); // alociranje memorije za poruku

		int cnt = 0;

		while (cnt < bytesExpected) {

			int selectResult = SelectFunction(acceptedSocket, 'r');
			if (selectResult == -1) {
				return "ErrorS";
			}

			iResult = recv(acceptedSocket, myBuffer + cnt, bytesExpected - cnt, 0);

			//printf("Message received from client: %s.\n", myBuffer);

			cnt += iResult;
		}
		return myBuffer;
	}
	else if (iResult == 0)
	{
		// connection was closed gracefully
		//printf("Connection with client closed.\n");
		//closesocket(acceptedSocket);
		return "ErrorC";
	}
	else
	{
		// there was an error during recv
		//printf("recv failed with error: %d\n", WSAGetLastError());
		//closesocket(acceptedSocket);
		return "ErrorR";
	}

}
int SendFunction(SOCKET connectSocket, char* message, int messageSize) {

	int selectResult = SelectFunction(connectSocket, 'w');
	if (selectResult == -1) {
		return -1;
	}
	int iResult = send(connectSocket, message, messageSize, 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return SOCKET_ERROR;
	}
	else {

		int cnt = iResult;
		while (cnt < messageSize) {

			SelectFunction(connectSocket, 'w');
			iResult = send(connectSocket, message + cnt, messageSize - cnt, 0);
			cnt += iResult;
		}
	}

	return 1;
	//printf("Bytes Sent: %ld\n", iResult);
}

int SelectFunction(SOCKET listenSocket, char rw) {
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

		if (!recvPossible) {
			return -1;
		}

		if (!sendPossible) {
			return -1;
		}

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

	return 1;

}

///<summary>
/// Printing menu for interaction with subscriber.
///</summary>
///<returns>No return value.</returns>
void PrintMenu() {
	printf("\nChoose a topic to subscribe to: \n");
	printf("\t1.Sport\n");
	printf("\t2.Fashion\n");
	printf("\t3.Politics\n");
	printf("\t4.News \n");
	printf("\t5.Show buisness \n\n");
	printf("Press X if you want to close connection\n");
}

///<summary>
/// Generates message to send to server depending on input.
///</summary>
///<param name ="input">Input from subscriber.</param>
///<param name ="message">Message to copy to.</param>
///<returns>No return value.</returns>
void ProcessInputAndGenerateMessage(char input, char* message) {
	switch (input) {
	case '1':
		strcpy(message, "s:Sport");
		printf("You subscribed on Sport.\n");
		break;
	case '2':
		strcpy(message, "s:Fashion");
		printf("You subscribed on Fashion.\n");
		break;
	case '3':
		strcpy(message, "s:Politics");
		printf("You subscribed on Politics.\n");
		break;
	case '4':
		strcpy(message, "s:News");
		printf("You subscribed on News.\n");
		break;
	case '5':
		strcpy(message, "s:Show business");
		printf("You subscribed on Show business.\n");
		break;
	default:
		break;
	}
}