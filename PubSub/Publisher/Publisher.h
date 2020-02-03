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
#define MAX_MESSAGE_SIZE 250

bool serverStopped = false;
bool appRunning = true;

void EnterAndGenerateMessage(char* , char* );
bool ValidateMessage(char* );
int SelectFunction(SOCKET, char);
void PrintMenu();
void ProcessInput(char, char* );
int SendFunction(SOCKET,char*,int);
char* ReceiveFunction(SOCKET, char*);
int Connect(SOCKET);


///<summary>
/// Validates message to publish.
///</summary>
///<param name ="publish_message">Message to be published.</param>
///<returns>Return false if message is empty, otherwise true.</returns>
bool ValidateMessage(char* publishMessage) {
	if (!strcmp(publishMessage, "\n")) {
		return false;
	}
	
	int messageLength = strlen(publishMessage);

	for (int i = 0; i < messageLength - 1; i++)
	{
		if (publishMessage[i] != ' ' && publishMessage[i] != '\t') {
			return true;
		}
	}
	return false;
}

///<summary>
/// Sending connection message to server.
///</summary>
///<param name ="connectSocket">Connected socket.</param>
///<returns>Return value of Send function(indicating error).</returns>
int Connect(SOCKET connectSocket) {
	char* connectMessage = (char*)malloc(10 * sizeof(char));
	strcpy(connectMessage, "p:Connect");

	int messageDataSize = strlen(connectMessage) + 1;
	int messageSize = messageDataSize + sizeof(int);

	MessageStruct* messageStruct = GenerateMessageStruct(connectMessage, messageDataSize);

	int retVal = SendFunction(connectSocket, (char*)messageStruct, messageSize);
	free(messageStruct);
	free(connectMessage);

	return retVal;

}

///<summary>
/// Sends a message through socket. Made for making sure the whole message has been sent.
///</summary>
///<param name ="connectSocket">Socket for sending message.</param>
///<param name ="message">Message to send.</param>
///<param name ="messageSize">Size of a message.</param>
///<returns>Return value of Select function if select is impossible, otherwise 0 or 1.</returns>
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

		int sentBytes = iResult;
		while (sentBytes < messageSize) {

			SelectFunction(connectSocket,'w');
			iResult = send(connectSocket, message + sentBytes, messageSize - sentBytes, 0);
			sentBytes += iResult;
		}
	}

	return 1;
}

///<summary>
/// Entering message to publish and generating a string to send to server.
///</summary>
///<param name ="publish_message">Publisher's input message.</param>
///<param name ="message">Generated message for sending.</param>
///<returns>No return value.</returns>
void EnterAndGenerateMessage(char* publishMessage, char* message)
{

	printf("Enter message you want to publish(max length: 250): \n");
	
	fgets(publishMessage, MAX_MESSAGE_SIZE, stdin);

	while (!ValidateMessage(publishMessage)) {

		printf("Message cannot be empty. Please enter your message again: \n");
		fgets(publishMessage, MAX_MESSAGE_SIZE, stdin);
	}


	if (strchr(publishMessage, '\n') == NULL) {
		int c;
		while ((c = fgetc(stdin)) != '\n' && c != EOF);
	}
	
	if ((strlen(publishMessage) > 0) && (publishMessage[strlen(publishMessage) - 1] == '\n'))
		publishMessage[strlen(publishMessage) - 1] = '\0';
	
	strcat(message, ":");
	strcat(message, publishMessage);

	printf("You published message: %s.\n", publishMessage);
}


///<summary>
/// Receives a message through socket. Made for making sure the whole message has been received.
///</summary>
///<param name ="acceptedSocket">Socket for receiving message.</param>
///<param name ="recvbuf">Buffer to receive message.</param>
///<returns>Received message. Eror type in case of error.</returns>
char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf) {

	int iResult;
	char* retVal = (char*)malloc(7 * sizeof(char));
	int selectResult = SelectFunction(acceptedSocket, 'r');
	if (selectResult == -1) {
		memcpy(retVal, "ErrorS", 7);	
		return retVal;
	}
	iResult = recv(acceptedSocket, recvbuf, 4, 0); 

    if (iResult == 0)
	{
		memcpy(retVal, "ErrorC", 7);	
	}
	else if(iResult == SOCKET_ERROR)
	{
		memcpy(retVal, "ErrorR", 7);
	}
	return retVal;
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
	do {
		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);

		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (!appRunning || serverStopped) {
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