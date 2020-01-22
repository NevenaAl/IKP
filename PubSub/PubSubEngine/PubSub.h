#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Queue.h"
#include "Dictionary.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define SERVER_SLEEP_TIME 50
#define NUMBER_OF_CLIENTS 40
#define SAFE_DELETE_HANDLE(h) {if(h)CloseHandle(h);}

bool InitializeWindowsSockets();
void SelectFunction(SOCKET, char);
void Subscribe(struct Queue*, SOCKET, char*);
void Publish(struct MessageQueue*, char*, char*);
void SubscriberShutDown(Queue*, SOCKET, struct node subscribers[]);
void ReceiveFunction(SOCKET acceptedSocket, char* recvbuf);
void SendFunction(SOCKET connectSocket, char* message, int messageSize);
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


void ReceiveFunction(SOCKET acceptedSocket, char* recvbuf) {

	int iResult;

	do {

		// Receive data until the client shuts down the connection
		SelectFunction(acceptedSocket, 'r');
		iResult = recv(acceptedSocket, recvbuf, 4, 0); // primamo samo header poruke

		if (iResult > 0)
		{
			int bytesExpected = *((int*)recvbuf);
			printf("Size of message is : %d\n", bytesExpected);

			char* myBuffer = (char*)(malloc(sizeof(bytesExpected))); // alociranje memorije za poruku

			int cnt = 0;

			while (cnt < bytesExpected) {

				SelectFunction(acceptedSocket, 'r');
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

void Subscribe(struct Queue* queue, SOCKET sub, char* topic) {
	for (int i = 0; i < queue->size; i++) {
		if (!strcmp(queue->array[i].topic, topic)) {
			int index = queue->array[i].size;
			queue->array[i].subs_array[index] = sub;
			queue->array[i].size++;
		}
	}
}

void Publish(struct MessageQueue* message_queue, char* topic, char* message) {
	
	topic_message item;
	memcpy(item.message, message, strlen(message)+1);
	memcpy(item.topic, topic, strlen(topic)+1);

	EnqueueMessageQueue(message_queue, item);

	printf("\nPublisher published message: %s to topic: %s\n", item.message, item.topic);
}
void SelectFunction(SOCKET listenSocket, char rw) {
	int iResult = 0;
	do {
		FD_SET set;
		timeval timeVal;

		FD_ZERO(&set);

		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

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

