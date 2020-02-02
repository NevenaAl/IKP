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


bool appRunning = true;

int SelectFunction(SOCKET, char);
void PrintMenu();
void ProcessInputAndGenerateMessage(char, char* );
int SendFunction(SOCKET, char*, int);
char* ReceiveFunction(SOCKET, char* );
bool AlreadySubscribed(char, int[], int);
int Connect(SOCKET);

///<summary>
/// Sending connection message to server.
///</summary>
///<param name ="connectSocket">Connected socket.</param>
///<returns>Return value of Send function(indicating error).</returns>
int Connect(SOCKET connectSocket) {
	char* connectMessage = (char*)malloc(10 * sizeof(char));
	strcpy(connectMessage, "s:Connect");

	int messageDataSize = strlen(connectMessage) + 1;
	int messageSize = messageDataSize + sizeof(int);

	MessageStruct* messageStruct = GenerateMessageStruct(connectMessage, messageDataSize);

	int retVal = SendFunction(connectSocket, (char*)messageStruct, messageSize);
	free(messageStruct);
	free(connectMessage);

	return retVal;

}

///<summary>
/// Checks if subscriber is already subscribed to certain topic.
///</summary>
///<param name ="c">Client's input.</param>
///<param name ="subscribed">Array of topics client is subscribed to.</param>
///<param name ="numOfSubscribedTopics">Number of topics client is subscribed to.</param>
///<returns>Returns true if client is subscribed to topic, otherwise false.</returns>
bool AlreadySubscribed(char c, int subscribed[], int numOfSubscribedTopics) {
	for (int i = 0; i < numOfSubscribedTopics; i++) {
		if (subscribed[i] == c - '0') {
			return true;
		}
	}
	return false;
}

///<summary>
/// Receives a message through socket. Made for making sure the whole message has been received.
///</summary>
///<param name ="acceptedSocket">Socket for receiving message.</param>
///<param name ="recvbuf">Buffer to receive message.</param>
///<returns>Received message. Eror type in case of error.</returns>
char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf) {

	int iResult;
	char* myBuffer = (char*)(malloc(DEFAULT_BUFLEN));

	int selectResult = SelectFunction(acceptedSocket, 'r');
	if (selectResult == -1) {
		memcpy(myBuffer, "ErrorS", 7);
		return myBuffer;
	}
	iResult = recv(acceptedSocket, recvbuf, 4, 0); 

	if (iResult > 0)
	{
		int bytesExpected = *((int*)recvbuf);

		int recvBytes = 0;

		while (recvBytes < bytesExpected) {

			SelectFunction(acceptedSocket, 'r');
			iResult = recv(acceptedSocket, myBuffer + recvBytes, bytesExpected - recvBytes, 0);

			recvBytes += iResult;
		}
	}
	else if (iResult == 0)
	{
		memcpy(myBuffer, "ErrorC", 7);
	}
	else
	{
		memcpy(myBuffer, "ErrorR", 7);
	}
	return myBuffer;

}

///<summary>
/// Sends a message through socket. Made for making sure the whole message has been sent.
///</summary>
///<param name ="connectSocket">Socket for sending message.</param>
///<param name ="message">Message to send.</param>
///<param name ="messageSize">Size of a message.</param>
///<returns>Return value of Select function if select is impossible, otherwise 0 or 1.</returns>
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

		int sentBytes = iResult;
		while (sentBytes < messageSize) {

			SelectFunction(connectSocket, 'w');
			iResult = send(connectSocket, message + sentBytes, messageSize - sentBytes, 0);
			sentBytes += iResult;
		}
	}

	return 1;
}

///<summary>
/// Select function used in nonblocking mode.
/// Waits until send or receive is possible.
///</summary>
///<param name ="lisenSocket">Socket put in FD_SET.</param>
///<param name ="rw">Char used to inform wich mode is used(read or write).</param>
///<returns>Returns -1 if server closed connection.</returns>
int SelectFunction(SOCKET listenSocket, char rw) {
	int iResult = 0;
	while(true) {
		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);
		
		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (!appRunning) {
			return -1;
		}


		if (rw == 'r') {
			iResult = select(0 /* ignored */, &set, NULL, NULL, &timeVal);
		}
		else {
			iResult = select(0 /* ignored */, NULL, &set, NULL, &timeVal);
		}


	
		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			continue;
		}

	
		if (iResult == 0)
		{
	
			Sleep(SERVER_SLEEP_TIME);
			continue;
		}
		break;
	
	}

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