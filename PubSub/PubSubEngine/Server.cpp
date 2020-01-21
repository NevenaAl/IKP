#include "PubSub.h"

CRITICAL_SECTION queueAccess;
CRITICAL_SECTION message_queueAccess;
HANDLE hSemaphore;

struct Queue* queue = CreateQueue(2);
struct MessageQueue* message_queue = CreateMessageQueue(1000);

DWORD WINAPI PublisherWork(LPVOID lpParam) 
{
	int iResult = 0;
	char recvbuf[DEFAULT_BUFLEN];
	SOCKET recvSocket = *(SOCKET*)lpParam;

	while (true) {
		SelectFunc(iResult, recvSocket, 'r');

		iResult = recv(recvSocket, recvbuf, DEFAULT_BUFLEN, 0);

		char delimiter[] = ":";
		char *ptr = strtok(recvbuf, delimiter);

		char *role = ptr;
		ptr = strtok(NULL, delimiter);
		char *topic = ptr;
		ptr = strtok(NULL, delimiter);
		char *message = ptr;

		if (!strcmp(role, "p")) {
			ptr = strtok(NULL, delimiter);
			EnterCriticalSection(&message_queueAccess);
			Publish(message_queue, topic, message);
			LeaveCriticalSection(&message_queueAccess);
			ReleaseSemaphore(hSemaphore, 1, NULL);
		}
	}
	return 1;
}
DWORD WINAPI SubscriberWork(LPVOID lpParam)
{
	//u ovom threadu send pa subscirber klijentu kad publisher nesto stavi u message queue
	//return 0;
	int iResult = 0;
	SOCKET sendSocket;

	WaitForSingleObject(hSemaphore, INFINITE);
	EnterCriticalSection(&message_queueAccess);
	topic_message item = DequeueMessageQueue(message_queue);
 	LeaveCriticalSection(&message_queueAccess);
	printf("Topic: %s\n", item.topic);
	printf("message: %s\n", item.message);
	
	/*char* messageToSend = item.message;
	int messagesize = strlen(messageToSend) + 1 + sizeof(int);
	char* bufferToSend = (char*)malloc(messagesize);
	char* toSend = bufferToSend;
	*((int*)bufferToSend) = int(strlen(messageToSend));
	bufferToSend+=4;
	memcpy((char*)bufferToSend, messageToSend, int(strlen(messageToSend)+1));*/

	int messageSize = int(strlen(item.message) + strlen(item.topic) + 2 + sizeof(int));
	printf("message size: %d", messageSize);
	char* toSend = (char*)malloc(messageSize);
	char* buffer = toSend;
	*((int*)toSend) = messageSize;
	toSend += 4;
	memcpy(toSend, (char*)&item, 4);
	char* structtosend = (char*)&item;

	//poslati svim pretplacenim
	for (int i = 0; i < queue->size; i++)
	{
		if (!strcmp(queue->array[i].topic, item.topic)) {
			for (int j = 0; j < queue->array[i].size; j++)
			{
				sendSocket = queue->array[i].subs_array[j];
				SelectFunc(iResult, sendSocket, 'w');
				iResult = send(sendSocket, (char*)&item, sizeof(topic_message), 0);
				if (iResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(sendSocket);
					WSACleanup();
					return 1;
				}

				printf("Bytes Sent: %ld\n", iResult);
			}
		}
		
	}

	
	return 1;
}

int  main(void)
{
	Enqueue(queue, "Sport");
	Enqueue(queue, "Fashion");
	Enqueue(queue, "Politics");
	Enqueue(queue, "News");
	Enqueue(queue, "Show business");

	int clientsCount = 0;
	int numberOfPublishers = 0;
	int numberOfSubscribers = 0;

	InitializeCriticalSection(&queueAccess);
	InitializeCriticalSection(&message_queueAccess);

	hSemaphore = CreateSemaphore(0, 0, 1, NULL);

	/*HANDLE ClientThreads[NUMBER_OF_CLIENTS];
	DWORD ClientThreadsID[NUMBER_OF_CLIENTS];*/

	HANDLE PublisherThreads[NUMBER_OF_CLIENTS];
	DWORD PublisherThreadsID[NUMBER_OF_CLIENTS]; 

	HANDLE SubscriberThreads[NUMBER_OF_CLIENTS];
	DWORD SubscriberThreadsID[NUMBER_OF_CLIENTS]; 
	
	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;
	// Socket used for communication with client
	SOCKET acceptedSockets[NUMBER_OF_CLIENTS];
	// variable used to store function return value
	int iResult;
	// Buffer used for storing incoming data
	char recvbuf[DEFAULT_BUFLEN];

	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	// Prepare address information structures
	addrinfo *resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       // IPv4 address
	hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
	hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
	hints.ai_flags = AI_PASSIVE;     // 

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(AF_INET,      // IPv4 address famly
		SOCK_STREAM,  // stream socket
		IPPROTO_TCP); // TCP

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address 
	// to socket
	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	//NEW
	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &nonBlockingMode);

	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}
	//NEW

	// Since we don't need resultingAddress any more, free it
	freeaddrinfo(resultingAddress);

	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server initialized, waiting for clients.\n");

	do
	{
		SelectFunc(iResult, listenSocket, 'r');

		acceptedSockets[clientsCount] = accept(listenSocket, NULL, NULL);

		if (acceptedSockets[clientsCount] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		SelectFunc(iResult, acceptedSockets[clientsCount], 'r');

		iResult = recv(acceptedSockets[clientsCount], recvbuf, DEFAULT_BUFLEN, 0);

		if (iResult > 0)
		{
			char delimiter[] = ":";
			char *ptr = strtok(recvbuf, delimiter);

			char *role = ptr;
			ptr = strtok(NULL, delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			char *message = ptr;

			if (!strcmp(role, "s")) {
				SubscriberThreads[numberOfSubscribers] = CreateThread(NULL, 0, &SubscriberWork, &acceptedSockets[clientsCount], 0, &SubscriberThreadsID[numberOfSubscribers]);
				EnterCriticalSection(&queueAccess);
				Subscribe(queue, acceptedSockets[clientsCount], topic);
				LeaveCriticalSection(&queueAccess);
				numberOfSubscribers++;
				printf("Subscriber subscribed to topic: %s. \n", topic);
			}

			if (!strcmp(role, "p")) {

				PublisherThreads[numberOfPublishers] = CreateThread(NULL, 0, &PublisherWork, &acceptedSockets[clientsCount], 0, &PublisherThreadsID[numberOfPublishers]);
				printf("Publisher connected.\n");
				numberOfPublishers++;
			}
		}
		else if (iResult == 0)
		{
			// connection was closed gracefully
			printf("Connection with client closed.\n");
			closesocket(acceptedSockets[clientsCount]);
		}
		else
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSockets[clientsCount]);
		}
		
		clientsCount++;

	} while (clientsCount < NUMBER_OF_CLIENTS);

	for (int i = 0; i < numberOfPublishers; i++) {

		if (PublisherThreads[i])
			WaitForSingleObject(PublisherThreads[i], INFINITE);
	}

	for (int i = 0; i < numberOfSubscribers; i++) {

		if (SubscriberThreads[i])
			WaitForSingleObject(SubscriberThreads[i], INFINITE);
	}

	printf("Server shutting down.");

	DeleteCriticalSection(&queueAccess);
	DeleteCriticalSection(&message_queueAccess);

	for (int i = 0; i < numberOfPublishers; i++) {
		SAFE_DELETE_HANDLE(PublisherThreads[i]);
	}

	for (int i = 0; i < numberOfSubscribers; i++) {
		SAFE_DELETE_HANDLE(SubscriberThreads[i]);
	}

	// shutdown the connection since we're done
	for (int i = 0; i < NUMBER_OF_CLIENTS; i++) {

		iResult = shutdown(acceptedSockets[clientsCount], SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSockets[clientsCount]);
			WSACleanup();
			return 1;
		}
		closesocket(acceptedSockets[i]);
	}
	// cleanup
	closesocket(listenSocket);
	
	WSACleanup();

	free(queue);
	free(message_queue);

	return 0;
}

