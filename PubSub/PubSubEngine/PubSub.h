#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "..\Common\SocketOperations.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define SERVER_SLEEP_TIME 50
#define NUMBER_OF_CLIENTS 40

bool serverRunning = true;
int clientsCount = 0;
int numberOfPublishers = 0;
int numberOfPublishers = 0;
int numberOfSubscribers = 0;
ThreadArgument publisherThreadArgument;
ThreadArgument subscriberThreadArgument;

int SelectFunction(SOCKET, char);
void Subscribe(struct Queue*, SOCKET, char*);
void Publish(struct MessageQueue*, char*, char*,int);
void SubscriberShutDown(Queue*, SOCKET, struct Subscriber subscribers[]);
char* ReceiveFunction(SOCKET acceptedSocket, char* recvbuf);
int SendFunction(SOCKET connectSocket, char* message, int messageSize);
char Connect(SOCKET);
void AddTopics(Queue*);



///<summary>
/// Adds toppics to queue.
///</summary>
///<param name ="queue">Pointer to queue.</param>
///<returns>No return value.</returns>
void AddTopics(Queue* queue) {

	Enqueue(queue, "Sport");
	Enqueue(queue, "Fashion");
	Enqueue(queue, "Politics");
	Enqueue(queue, "News");
	Enqueue(queue, "Show business");
}

///<summary>
/// Receiving connection message from clients. 
///</summary>
///<param name ="acceptedSocket">Accepted socket.</param>
///<returns>Type of connected client('p' or 's').</returns>
char Connect(SOCKET acceptedSocket) {
	char recvbuf[DEFAULT_BUFLEN];
	char *recvRes;
	//memcpy(recvbuf, ReceiveFunction(acceptedSocket, recvbuf), DEFAULT_BUFLEN);
	recvRes = ReceiveFunction(acceptedSocket, recvbuf);
	if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR"))
	{
		//char delimiter[] = ":";
		char delimiter = ':';
		char *ptr = strtok(recvRes, &delimiter);

		char *role = ptr;
		ptr = strtok(NULL, &delimiter);

		if (!strcmp(role, "s")) {
			
			subscriberThreadArgument.ordinalNumber = numberOfSubscribers;
			subscriberThreadArgument.socket = acceptedSocket;
			subscriberThreadArgument.clientNumber = clientsCount;
			//SubscriberThreads[numberOfSubscribers] = CreateThread(NULL, 0, &SubscriberReceive, &argumentStructure, 0, &SubscriberThreadsID[numberOfSubscribers]);
			printf("\nSubscriber %d connected.\n", numberOfSubscribers);
			
			free(recvRes);
			return 's';
		}

		if (!strcmp(role, "p")) {
			publisherThreadArgument.ordinalNumber = numberOfPublishers;
			publisherThreadArgument.socket = acceptedSocket;
			publisherThreadArgument.clientNumber = clientsCount;
			//PublisherThreads[numberOfPublishers] = CreateThread(NULL, 0, &PublisherWork, &acceptedSocket, 0, &PublisherThreadsID[numberOfPublishers]);
			printf("\nPublisher %d connected.\n", numberOfPublishers);
			
			free(recvRes);
			return 'p';
		}
		
	}
	else if (!strcmp(recvRes, "ErrorC"))
	{
		// connection was closed gracefully
		printf("\nConnection with client closed.\n");
		closesocket(acceptedSocket);
	}
	else if (!strcmp(recvRes, "ErrorR"))
	{
		// there was an error during recv
		printf("\nrecv failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
	}
	free(recvRes);
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
		printf("\nsend failed with error: %d\n", WSAGetLastError());
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
	char* myBuffer = (char*)(malloc(DEFAULT_BUFLEN));

		int selectResult = SelectFunction(acceptedSocket, 'r');
		if (selectResult == -1) {
			memcpy(myBuffer, "ErrorS", 7);
			return myBuffer;
		}
		iResult = recv(acceptedSocket, recvbuf, 4, 0); // primamo samo header poruke

		if (iResult > 0)
		{
			int bytesExpected = *((int*)recvbuf);
			//printf("Size of message is : %d\n", bytesExpected);

			//char* myBuffer = (char*)(malloc(bytesExpected)); // alociranje memorije za poruku

			int cnt = 0;

			while (cnt < bytesExpected) {

				SelectFunction(acceptedSocket, 'r');
				iResult = recv(acceptedSocket, myBuffer + cnt, bytesExpected - cnt, 0);

				//printf("Message received from client: %s.\n", myBuffer);

				cnt += iResult;
			}
			//return myBuffer;
		}
		else if (iResult == 0)
		{
			// connection was closed gracefully
			//printf("Connection with client closed.\n");
			//closesocket(acceptedSocket);
			memcpy(myBuffer, "ErrorC", 7);
		}
		else
		{
			// there was an error during recv
			//printf("recv failed with error: %d\n", WSAGetLastError());
			//closesocket(acceptedSocket);
			memcpy(myBuffer, "ErrorR", 7);
		}	
		return(myBuffer);
		
}

///<summary>
/// Deletes subscriber from queue when he closes connection.
/// Prevents trying to send a message on closed socket.
///</summary>
///<param name ="queue">Queue to delete from.</param>
///<param name ="acceptedSocket">Subscriber's socket.</param>
///<param name ="subscribers">Array of Subscribers(socket + semaphore).</param>
///<returns>No return value.</returns>
void SubscriberShutDown(Queue* queue, SOCKET acceptedSocket, struct Subscriber subscribers[]) {
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

	for (int i = 0; i < sizeof(subscribers)/sizeof(struct Subscriber); i++)
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
void Publish(struct MessageQueue* message_queue, char* topic, char* message,int ordinalNumber) {
	
	struct topic_message item;
	memcpy(item.message, message, strlen(message)+1);
	memcpy(item.topic, topic, strlen(topic)+1);

	EnqueueMessageQueue(message_queue, item);

	printf("\nPublisher %d published message: %s to topic: %s\n",ordinalNumber, item.message, item.topic);

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
			fprintf(stderr, "\nselect failed with error: %ld\n", WSAGetLastError());
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



