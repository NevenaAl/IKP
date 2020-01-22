#include "PubSub.h"
#include "Dictionary.h"

CRITICAL_SECTION queueAccess;
CRITICAL_SECTION message_queueAccess;

HANDLE pubSubSemaphore;


struct Queue* queue = CreateQueue(2);
struct MessageQueue* message_queue = CreateMessageQueue(1000);
topic_message current;
struct node subscribers[20];

struct ThreadArgument {
	SOCKET socket;
	int numberOfSubs;
};

HANDLE SubscriberThreads[NUMBER_OF_CLIENTS];
DWORD SubscriberThreadsID[NUMBER_OF_CLIENTS];

DWORD WINAPI SubscriberWork(LPVOID lpParam)
{
	int iResult = 0;
	SOCKET sendSocket = *(SOCKET*)lpParam;
	while (true) {
		for (int i = 0; i < (sizeof(subscribers) / sizeof(node)); i++)
		{
			if (sendSocket == subscribers[i].sendTo) {
				WaitForSingleObject(subscribers[i].hSemaphore, INFINITE);
				break;
			}
		}
		
		SelectFunction(*(SOCKET*)lpParam, 'w');
		iResult = send(*(SOCKET*)lpParam, (char*)&current, sizeof(topic_message), 0);
		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(sendSocket);
			WSACleanup();
			return 1;
		}
		//SendFunction(*(SOCKET*)lpParam, (char*)&current, sizeof(topic_message));

		//printf("Bytes Sentttt: %ld\n", iResult);
	}
	return 1;
}

DWORD WINAPI SubscriberReceive(LPVOID lpParam) {
	char recvbuf[DEFAULT_BUFLEN];
	ThreadArgument argumentStructure = *(ThreadArgument*)lpParam;
	
	SelectFunction(argumentStructure.socket, 'r');

	int iResult = recv(argumentStructure.socket, recvbuf, DEFAULT_BUFLEN, 0);
	if (iResult > 0)
	{
		char delimiter[] = ":";
		char *ptr = strtok(recvbuf, delimiter);

		char *role = ptr;
		ptr = strtok(NULL, delimiter);
		char *topic = ptr;
		ptr = strtok(NULL, delimiter);
		if (!strcmp(topic, "shutDown")) {
			SubscriberShutDown(queue, argumentStructure.socket);
		}
		HANDLE hSem = CreateSemaphore(0, 0, 1, NULL);

		struct node subscriber;
		subscriber.sendTo = argumentStructure.socket;
		subscriber.hSemaphore = hSem;
		subscribers[argumentStructure.numberOfSubs] = subscriber;

		SubscriberThreads[argumentStructure.numberOfSubs] = CreateThread(NULL, 0, &SubscriberWork, &argumentStructure.socket, 0, &SubscriberThreadsID[argumentStructure.numberOfSubs]);

		EnterCriticalSection(&queueAccess);
		Subscribe(queue, argumentStructure.socket, topic);
		LeaveCriticalSection(&queueAccess);
		//numberOfSubscribers++;
		printf("Subscriber %d subscribed to topic: %s. \n",argumentStructure.numberOfSubs, topic);
		}
		else if (iResult == 0)
		{
			printf("Connection with client closed.\n");
			closesocket(argumentStructure.socket);
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentStructure.socket);

		}
	while (true) {
		SelectFunction(argumentStructure.socket, 'r');

		int iResult = recv(argumentStructure.socket, recvbuf, DEFAULT_BUFLEN, 0);
		if (iResult > 0)
		{
			char delimiter[] = ":";
			char *ptr = strtok(recvbuf, delimiter);

			char *role = ptr;
			ptr = strtok(NULL, delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			if (!strcmp(topic, "shutDown")) {
				SubscriberShutDown(queue, argumentStructure.socket);
			}
			
			EnterCriticalSection(&queueAccess);
			Subscribe(queue, argumentStructure.socket, topic);
			LeaveCriticalSection(&queueAccess);
			//numberOfSubscribers++;
			printf("Subscriber %d subscribed to topic: %s. \n", argumentStructure.numberOfSubs, topic);
		}
		else if (iResult == 0)
		{
			// connection was closed gracefully
			printf("Connection with client closed.\n");
			closesocket(argumentStructure.socket);
			break;
		}
		else
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentStructure.socket);
			break;

		}
	}
	return 1;
}
DWORD WINAPI PubSubWork(LPVOID lpParam) {
	//u ovom threadu send pa subscirber klijentu kad publisher nesto stavi u message queue
	//return 0;
	int iResult = 0;
	SOCKET sendSocket;
	while (true) {
		WaitForSingleObject(pubSubSemaphore, INFINITE);

		EnterCriticalSection(&message_queueAccess);
		current = DequeueMessageQueue(message_queue);
		LeaveCriticalSection(&message_queueAccess);

		printf("Topic: %s\n", current.topic);
		printf("message: %s\n\n", current.message);

		int numOfSockets = 0;
		//poslati svim pretplacenim
		for (int i = 0; i < queue->size; i++)
		{
			if (!strcmp(queue->array[i].topic, current.topic)) {
				for (int j = 0; j < queue->array[i].size; j++)
				{
					sendSocket = queue->array[i].subs_array[j];
					for (int i = 0; i < (sizeof(subscribers) / sizeof(node)); i++)
					{
						if (subscribers[i].sendTo == sendSocket) {
							ReleaseSemaphore(subscribers[i].hSemaphore, 1, NULL);
							break;
						}
					}
				}
			}

		}

	}
	return 1;
}

DWORD WINAPI PublisherWork(LPVOID lpParam) 
{
	int iResult = 0;
	char recvbuf[DEFAULT_BUFLEN];
	SOCKET recvSocket = *(SOCKET*)lpParam;

	while (true) {
		SelectFunction(recvSocket, 'r');

		iResult = recv(recvSocket, recvbuf, DEFAULT_BUFLEN, 0);
		//ReceiveFunction(recvSocket, recvbuf);

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
			ReleaseSemaphore(pubSubSemaphore, 1, NULL);
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

	pubSubSemaphore = CreateSemaphore(0, 0, 1, NULL);

	/*HANDLE ClientThreads[NUMBER_OF_CLIENTS];
	DWORD ClientThreadsID[NUMBER_OF_CLIENTS];*/

	HANDLE PublisherThreads[NUMBER_OF_CLIENTS];
	DWORD PublisherThreadsID[NUMBER_OF_CLIENTS];

	

	HANDLE pubSubThread;
	DWORD pubSubThreadID;

	pubSubThread = CreateThread(NULL, 0, &PubSubWork, NULL, 0, &pubSubThreadID);

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
		SelectFunction(listenSocket, 'r');

		acceptedSockets[clientsCount] = accept(listenSocket, NULL, NULL);

		if (acceptedSockets[clientsCount] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}


			SelectFunction(acceptedSockets[clientsCount], 'r');

			iResult = recv(acceptedSockets[clientsCount], recvbuf, DEFAULT_BUFLEN, 0);
		   //ReceiveFunction(acceptedSockets[clientsCount], recvbuf);

			if (iResult > 0)
			{
				char delimiter[] = ":";
				char *ptr = strtok(recvbuf, delimiter);

				char *role = ptr;
				ptr = strtok(NULL, delimiter);
				//char *topic = ptr;
				//ptr = strtok(NULL, delimiter);
				//char *message = ptr;

				if (!strcmp(role, "s")) {
					ThreadArgument argumentStructure;
					argumentStructure.numberOfSubs = numberOfSubscribers;
					argumentStructure.socket = acceptedSockets[clientsCount];
					SubscriberThreads[numberOfSubscribers] = CreateThread(NULL, 0, &SubscriberReceive, &argumentStructure, 0, &SubscriberThreadsID[numberOfSubscribers]);
					printf("Subscriber %d connected.\n", numberOfSubscribers);
					numberOfSubscribers++;

				}

				if (!strcmp(role, "p")) {

					PublisherThreads[numberOfPublishers] = CreateThread(NULL, 0, &PublisherWork, &acceptedSockets[clientsCount], 0, &PublisherThreadsID[numberOfPublishers]);
					printf("Publisher %d connected.\n",numberOfPublishers);
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

