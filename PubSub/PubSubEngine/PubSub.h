#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Queue.h"
#include "Dictionary.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define SERVER_SLEEP_TIME 50
#define NUMBER_OF_CLIENTS 40
#define SAFE_DELETE_HANDLE(h) {if(h)CloseHandle(h);}


bool serverRunning = true;

struct MessageStruct
{
	int header;
	char message[DEFAULT_BUFLEN - 4];

}typedef MessageStruct;



bool InitializeWindowsSockets();
int SelectFunction(SOCKET, char);
void Subscribe(struct Queue*, SOCKET, char*);
void Publish(struct MessageQueue*, char*, char*);
void SubscriberShutDown(Queue*, SOCKET, struct node subscribers[]);
char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf);
int SendFunction(SOCKET connectSocket, char* message, int messageSize);
MessageStruct* GenerateMessageStruct(char* message, int len);

///<summary>
/// Generates Message Struct with a header containing the lenght of a message.
///</summary>
///<param name ="message">Message.</param>
///<param name ="len">Length of a message.</param>
///<returns>Message Struct created.</returns>
MessageStruct* GenerateMessageStruct(char* message, int len) {

	MessageStruct* messageStruct = (MessageStruct *)(malloc(sizeof(MessageStruct)));

	messageStruct->header = len;
	memcpy(messageStruct->message, message, len);
	return messageStruct;

}

///<summary>
/// Sends a message through socket. Made for making sure the whole message has been sent.
///</summary>
///<param name ="connectSocket">Socket for sending message.</param>
///<param name ="message">Message to send.</param>
///<param name ="messageSize">Size of a message.</param>
///<returns>No return value.</returns>
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
		return 0;
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

///<summary>
/// Receives a message through socket. Made for making sure the whole message has been received.
///</summary>
///<param name ="acceptedSocket">Socket for receiving message.</param>
///<param name ="recvbuf">Buffer to receive message.</param>
///<returns>Received message. Eror type in case of error.</returns>
char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf) {

	int iResult;

		int selectResult = SelectFunction(acceptedSocket, 'r');
		if (selectResult == -1) {
			return "ErrorS";
		}
		iResult = recv(acceptedSocket, recvbuf, 4, 0); // primamo samo header poruke

		if (iResult > 0)
		{
			int bytesExpected = *((int*)recvbuf);
			//printf("Size of message is : %d\n", bytesExpected);

			char* myBuffer = (char*)(malloc(bytesExpected)); // alociranje memorije za poruku

			int cnt = 0;

			while (cnt < bytesExpected) {

				SelectFunction(acceptedSocket, 'r');
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

///<summary>
/// Deletes subscriber from queue when he closes connection.
/// Prevents trying to send a message on closed socket.
///</summary>
///<param name ="queue">Queue to delete from.</param>
///<param name ="acceptedSocket">Subscriber's socket.</param>
///<param name ="subscribers">Array of nodes(socket + semaphore).</param>
///<returns>No return value.</returns>
void SubscriberShutDown(Queue* queue, SOCKET acceptedSocket, struct node subscribers[]) {
	for (int i = 0; i < queue->size; i++)
	{
		for (int j = 0; j < queue->array[i].size; j++)
		{
			if (queue->array[i].subs_array[j] == acceptedSocket) {
				int size = queue->array[i].size;
				SOCKET temp = queue->array[i].subs_array[size];
				if (temp != 3435973836) {
					queue->array[i].subs_array[size] = 3435973836;
					queue->array[i].subs_array[j] = temp;
					queue->array[i].size--;
				}
				else {
					queue->array[i].subs_array[j] = 3435973836;
					queue->array[i].size--;
				}
				
			}
		}

	}

	for (int i = 0; i < sizeof(subscribers)/sizeof(struct node); i++)
	{
		if (subscribers[i].sendTo == acceptedSocket) {
			subscribers[i].sendTo = 0;
			subscribers[i].hSemaphore = 0;
		}
	}
}

///<summary>
/// Puts subscriber in queue when he subscribes on certain topic.
///</summary>
///<param name ="queue">Queue to add to.</param>
///<param name ="sub">Subscriber's socket.</param>
///<param name ="topic">Topic subscriber has subscribed to.</param>
///<returns>No return value.</returns>
void Subscribe(struct Queue* queue, SOCKET sub, char* topic) {
	for (int i = 0; i < queue->size; i++) {
		if (!strcmp(queue->array[i].topic, topic)) {
			int index = queue->array[i].size;
			queue->array[i].subs_array[index] = sub;
			queue->array[i].size++;
		}
	}
}

///<summary>
/// Puts message on message queue when publihser publishes on certain topic.
///</summary>
///<param name ="message_queue">Queue to add to.</param>
///<param name ="message">Published message.</param>
///<param name ="topic">Topic publisher has published to.</param>
///<returns>No return value.</returns>
void Publish(struct MessageQueue* message_queue, char* topic, char* message) {
	
	topic_message item;
	memcpy(item.message, message, strlen(message)+1);
	memcpy(item.topic, topic, strlen(topic)+1);

	EnqueueMessageQueue(message_queue, item);

	printf("\nPublisher published message: %s to topic: %s\n", item.message, item.topic);

}


///<summary>
/// Select function used in nonblocking mode.
/// Waits until send or receive is possible.
///</summary>
///<param name ="lisenSocket">Socket put in FD_SET.</param>
///<param name ="rw">Char used to inform wich mode is used(read or write).</param>
///<returns>No return value.</returns>
int SelectFunction(SOCKET listenSocket, char rw) {
	int iResult = 0;
	do {
		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);

		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (!serverRunning)
			return -1;

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
/// Initializes sockets.
///</summary>
///<returns>Error bool.</returns>
bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

