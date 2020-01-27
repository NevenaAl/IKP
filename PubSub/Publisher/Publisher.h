#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016
#define SERVER_SLEEP_TIME 50

struct MessageStruct
{
	int header;
	char message[DEFAULT_BUFLEN - 4];

}typedef MessageStruct;


bool serverStopped = false;

bool InitializeWindowsSockets();
void EnterAndGenerateMessage(char* publish_message, char* message);
int SelectFunction(SOCKET, char);
void PrintMenu();
void ProcessInput(char input, char* message);
int SendFunction(SOCKET,char*,int);
MessageStruct* GenerateMessageStruct(char* message, int len);

MessageStruct* GenerateMessageStruct(char* message, int len) {

	MessageStruct* messageStruct = (MessageStruct *)(malloc(sizeof(MessageStruct)));

	messageStruct->header = len;
	memcpy(messageStruct->message, message, len);

	return messageStruct;

}

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

void EnterAndGenerateMessage(char* publish_message, char* message)
{

	printf("Enter message you want to publish:\n");
	scanf("%s", publish_message);

	strcat(message, ":");
	strcat(message, publish_message);

	printf("You published message: %s.\n", publish_message);
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

void PrintMenu() {
	printf("\nChoose a topic to publish to: \n");
	printf("\t1.Sport\n");
	printf("\t2.Fashion\n");
	printf("\t3.Politics\n");
	printf("\t4.News \n");
	printf("\t5.Show buisness \n\n");
	printf("Press X if you want to close connection\n");
}

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