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

bool serverStopped = false;

void EnterAndGenerateMessage(char* publish_message, char* message);
int SelectFunction(SOCKET, char);
void PrintMenu();
void ProcessInput(char input, char* message);
int SendFunction(SOCKET,char*,int);


int SendFunction(SOCKET connectSocket, char* message, int messageSize) {

	int selectResult = SelectFunction(connectSocket,'w');
	if (selectResult == -1) {
		return -1;
	}
	int iResult = send(connectSocket, message, messageSize, 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 0;
	}
	else {

		int cnt = iResult;
		while (cnt < messageSize) {

			SelectFunction(connectSocket,'w');
			iResult = send(connectSocket, message + cnt, messageSize - cnt, 0);
			cnt += iResult;
		}
	}

	//printf("Bytes Sent: %ld\n", iResult);
	return 1;
}

///<summary>
/// Entering message to publish and generating a string to send to server.
///</summary>
///<param name ="publish_message">Publisher's input message.</param>
///<param name ="message">Generated message for sending.</param>
///<returns>No return value.</returns>
void EnterAndGenerateMessage(char* publish_message, char* message)
{

	printf("Enter message you want to publish(max length: 250): \n");
	int c;
	scanf("%249s", publish_message);
	
	while ((c = fgetc(stdin)) != '\n' && c != EOF);

	strcat(message, ":");
	strcat(message, publish_message);

	printf("You published message: %s.\n", publish_message);
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

		if (serverStopped)
			return -1;

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

///<summary>
/// Printing Menu for interaction with publisher.
///</summary>
///<returns>No return value.</returns>
void PrintMenu() {
	printf("\nChoose a topic to publish to: \n");
	printf("\t1.Sport\n");
	printf("\t2.Fashion\n");
	printf("\t3.Politics\n");
	printf("\t4.News \n");
	printf("\t5.Show buisness \n\n");
	printf("Press X if you want to close connection\n");
}

///<summary>
/// Creates a new string depending on publisher's input.
///</summary>
///<param name ="input">Publisher's input.</param>
///<param name ="rw">Message to copy to.</param>
///<returns>No return value.</returns>
void ProcessInput(char input, char* message) {
	if (input == '1') {
		strcpy(message, "p:Sport");
	}
	else if (input == '2') {
		strcpy(message, "p:Fashion");
	}
	else if (input == '3') {
		strcpy(message, "p:Politics");
	}
	else if (input == '4') {
		strcpy(message, "p:News");
	}
	else if (input == '5') {
		strcpy(message, "p:Show business");
	}
}