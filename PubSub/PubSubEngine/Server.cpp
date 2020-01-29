#include "PubSub.h"

CRITICAL_SECTION queueAccess;
CRITICAL_SECTION message_queueAccess;

HANDLE pubSubSemaphore;
int publisherThreadKilled = -1;
int subscriberSendThreadKilled = -1;
int subscriberRecvThreadKilled = -1;
SOCKET acceptedSockets[NUMBER_OF_CLIENTS];

struct Queue* queue = CreateQueue(10);
struct MessageQueue* message_queue = CreateMessageQueue(10);
struct topic_message current;
struct Subscriber subscribers[20];


HANDLE SubscriberSendThreads[NUMBER_OF_CLIENTS];
DWORD SubscriberSendThreadsID[NUMBER_OF_CLIENTS];

HANDLE SubscriberRecvThreads[NUMBER_OF_CLIENTS];
DWORD SubscriberRecvThreadsID[NUMBER_OF_CLIENTS];

HANDLE PublisherThreads[NUMBER_OF_CLIENTS];
DWORD PublisherThreadsID[NUMBER_OF_CLIENTS];


///<summary>
/// A function executing in thread created and run at the beginning of the main program.
/// It is used for checking if thread is killed and closing its handle.
///</summary>
///<param name ="lpParam"></param>
///<returns>No return value.</returns>
DWORD WINAPI CloseHandles(LPVOID lpParam) {
	while (serverRunning) {
		if (publisherThreadKilled != -1) {
			for (int i = 0; i < numberOfPublishers; i++) {
				if (publisherThreadKilled == i) {
					if (PublisherThreads[i] != INVALID_HANDLE_VALUE) {
						SAFE_DELETE_HANDLE(PublisherThreads[i]);
						PublisherThreads[i] = 0;
						publisherThreadKilled = -1;
					}
					

				}
			}
				
		}
		if (subscriberSendThreadKilled != -1) {
			for (int i = 0; i < numberOfSubscribedSubs; i++) {
				if (subscriberSendThreadKilled == i) {
					SAFE_DELETE_HANDLE(SubscriberSendThreads[i]);
					SubscriberSendThreads[i] = 0;
					subscriberSendThreadKilled = -1;

				}
			}

		}

		if (subscriberRecvThreadKilled != -1) {
			for (int i = 0; i < numberOfConnectedSubs; i++) {
				if (subscriberRecvThreadKilled == i) {
					SAFE_DELETE_HANDLE(SubscriberRecvThreads[i]);
					SubscriberRecvThreads[i] = 0;
					subscriberRecvThreadKilled = -1;

				}
			}

		}
	}
	return 1;
}
///<summary>
/// A function executing in thread created and run at the beginning of the main program.
/// It is used for receiving input that causes closing server application.
///</summary>
///<param name ="lpParam"></param>
///<returns>No return value.</returns>
DWORD WINAPI GetChar(LPVOID lpParam)
{
	char c;
	while(serverRunning) {
		printf("\nIf you want to quit please press X!\n");
		 c = _getch();
		 if (c == 'x' || c == 'X') {
			 serverRunning = false;
			 ReleaseSemaphore(pubSubSemaphore, 1, NULL);
			 for (int i = 0; i < sizeof(subscribers); i++)
			 {
				 ReleaseSemaphore(subscribers[i].hSemaphore, 1, NULL);
			 }
			 int iResult = 0;
			 for (int i = 0; i < clientsCount; i++) {
				 //ako se klijent ugasio
				 if (acceptedSockets[i] != -1) {
					 iResult = shutdown(acceptedSockets[i], SD_BOTH);
					 if (iResult == SOCKET_ERROR)
					 {
						 printf("\nshutdown failed with error: %d\n", WSAGetLastError());
						 closesocket(acceptedSockets[i]);
						 WSACleanup();
						 return 1;
					 }
					 closesocket(acceptedSockets[i]);
				 } 
			 }
			 closesocket(*(SOCKET*)lpParam);
			 //WSACleanup();

			 break;
		 }

	} 
	return 1;
}

///<summary>
/// A function executing in thread created and run for each individual subscriber after he subscribes to a topic. 
/// It is used for forwarding messages published by publisher and put on message queue.
///</summary>
///<param name ="lpParam"> Subscriber's socket.</param>
///<returns> No return value.</returns>
DWORD WINAPI SubscriberWork(LPVOID lpParam)
{
	int iResult = 0;
	ThreadArgument argumentStructure = *(ThreadArgument*)lpParam;

	while (serverRunning) {
		for (int i = 0; i < (sizeof(subscribers) / sizeof(Subscriber)); i++)
		{
			if (argumentStructure.socket == subscribers[i].sendTo) {
				WaitForSingleObject(subscribers[i].hSemaphore, INFINITE);
				break;
			}
		}
		
		if (!serverRunning)
			break;

		char* message = (char*)malloc(sizeof(topic_message) + 1);
		memcpy(message, &current.topic, (strlen(current.topic)));
		memcpy(message + (strlen(current.topic)), ":", 1);
		memcpy(message + (strlen(current.topic) + 1), &current.message, (strlen(current.message) + 1));

		int messageDataSize = strlen(message) + 1;
		int messageSize = messageDataSize + sizeof(int);

		MessageStruct* messageStruct = GenerateMessageStruct(message, messageDataSize);
		int sendResult = SendFunction(argumentStructure.socket, (char*)messageStruct, messageSize);

		free(message);
		free(messageStruct);

		if (sendResult == -1)
			break;
	}
	subscriberSendThreadKilled = argumentStructure.ordinalNumber;
	return 1;
}

///<summary>
/// A function executing in thread created and run for each individual subscriber when he's connected. 
/// It is used for receiving messages from subscriber.
/// Enables subscribing on multiple topics.
///</summary>
///<param name ="lpParam"> A ThreadArgument structure that contains of a subscriber's socket and his ordinal number. </param>
///<returns>No return value.</returns>
DWORD WINAPI SubscriberReceive(LPVOID lpParam) {
	char recvbuf[DEFAULT_BUFLEN];
	ThreadArgument argumentRecvStructure = *(ThreadArgument*)lpParam;
	ThreadArgument argumentSendStructure = argumentRecvStructure;
	argumentSendStructure.ordinalNumber = numberOfSubscribedSubs;

	bool subscriberRunning = true;
	char* recvRes;

	//memcpy(recvbuf,ReceiveFunction(argumentStructure.socket, recvbuf), DEFAULT_BUFLEN);
	recvRes = ReceiveFunction(argumentSendStructure.socket, recvbuf);

	if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
	{
		//char delimiter[] = ":";
		char delimiter = ':';
		char *ptr = strtok(recvRes, &delimiter);

		char *role = ptr;
		ptr = strtok(NULL, &delimiter);
		char *topic = ptr;
		ptr = strtok(NULL, &delimiter);
		if (!strcmp(topic, "shutDown")) {
			printf("\nSubscriber %d disconnected.\n", argumentSendStructure.ordinalNumber);
			SubscriberShutDown(queue, argumentSendStructure.socket, subscribers);
			subscriberRunning = false;
			acceptedSockets[argumentSendStructure.clientNumber] = -1;
			free(recvRes);
			subscriberRecvThreadKilled = argumentSendStructure.ordinalNumber;
			return 1;
		}
		else {
			HANDLE hSem = CreateSemaphore(0, 0, 1, NULL);

			struct Subscriber subscriber;
			subscriber.sendTo = argumentSendStructure.socket;
			subscriber.hSemaphore = hSem;
			subscribers[argumentSendStructure.ordinalNumber] = subscriber;

			SubscriberSendThreads[numberOfSubscribedSubs] = CreateThread(NULL, 0, &SubscriberWork, &argumentSendStructure, 0, &SubscriberSendThreadsID[numberOfSubscribedSubs]);
			numberOfSubscribedSubs++;

			EnterCriticalSection(&queueAccess);
			Subscribe(queue, argumentSendStructure.socket, topic);
			LeaveCriticalSection(&queueAccess);
			printf("\nSubscriber %d subscribed to topic: %s. \n", argumentSendStructure.ordinalNumber, topic);
			free(recvRes);
		}
	}
	else if (!strcmp(recvRes, "ErrorS")) {
		free(recvRes);
		return 1;
	}
	else if (!strcmp(recvRes, "ErrorC"))
	{
			printf("\nConnection with client closed.\n");
			closesocket(argumentSendStructure.socket);
			free(recvRes);
	}
	else if (!strcmp(recvRes, "ErrorR"))
	{
			printf("\nrecv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentSendStructure.socket);
			free(recvRes);

	}

	while (subscriberRunning && serverRunning) {

		//memcpy(recvbuf, ReceiveFunction(argumentStructure.socket, recvbuf), DEFAULT_BUFLEN);;
		recvRes = ReceiveFunction(argumentSendStructure.socket, recvbuf);

		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			//char delimiter[] = ":";
			char delimiter = ':';
			char *ptr = strtok(recvRes, &delimiter);

			char *role = ptr;
			ptr = strtok(NULL, &delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, &delimiter);
			if (!strcmp(topic, "shutDown")) {
				printf("\nSubscriber %d disconnected.\n", argumentSendStructure.ordinalNumber);
				SubscriberShutDown(queue, argumentSendStructure.socket, subscribers);
				subscriberRunning = false;
				acceptedSockets[argumentSendStructure.clientNumber] = -1;
				free(recvRes);
				break;
			}
			
			EnterCriticalSection(&queueAccess);
			Subscribe(queue, argumentSendStructure.socket, topic);
			LeaveCriticalSection(&queueAccess);
			printf("\nSubscriber %d subscribed to topic: %s.\n", argumentSendStructure.ordinalNumber, topic);
			free(recvRes);

		}
		else if (!strcmp(recvRes, "ErrorS")) {
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
			// connection was closed gracefully
			printf("\nConnection with client closed.\n");
			closesocket(argumentSendStructure.socket);
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			// there was an error during recv
			printf("\nrecv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentSendStructure.socket);
			free(recvRes);
			break;

		}
	}
	subscriberRecvThreadKilled = argumentSendStructure.ordinalNumber;
	return 1;
}

///<summary>
/// A function executing in thread created and run at the begining of the main program. 
/// It is used for going through queue and message queue and determining which message has to be sent to which subscriber.
///</summary>
///<param name ="lpParam"></param>
///<returns>No return value.</returns>
DWORD WINAPI PubSubWork(LPVOID lpParam) {
	//u ovom threadu send pa subscirber klijentu kad publisher nesto stavi u message queue
	//return 0;
	int iResult = 0;
	SOCKET sendSocket;
	while (serverRunning) {
		WaitForSingleObject(pubSubSemaphore, INFINITE);
		if (!serverRunning)
			break;

		EnterCriticalSection(&message_queueAccess);
		current = DequeueMessageQueue(message_queue);
		LeaveCriticalSection(&message_queueAccess);

		int numOfSockets = 0;
		//poslati svim pretplacenim
		for (int i = 0; i < queue->size; i++)
		{
			if (!strcmp(queue->array[i].topic, current.topic)) {
				for (int j = 0; j < queue->array[i].size; j++)
				{
					sendSocket = queue->array[i].subs_array[j];
					for (int i = 0; i < (sizeof(subscribers) / sizeof(Subscriber)); i++)
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

///<summary>
/// A function executing in thread created and run for each individual publisher when he's connected. 
/// It is used for receiving messages from publisher.
///</summary>
///<param name ="lpParam"> Publisher's socket. </param>
///<returns>No return value.</returns>
DWORD WINAPI PublisherWork(LPVOID lpParam) 
{
	int iResult = 0;
	char recvbuf[DEFAULT_BUFLEN];
	ThreadArgument argumentStructure = *(ThreadArgument*)lpParam;
	char* recvRes;
	while (serverRunning) {

		//memcpy(recvbuf, ReceiveFunction(argumentStructure.socket, recvbuf), DEFAULT_BUFLEN);
		recvRes = ReceiveFunction(argumentStructure.socket, recvbuf);
		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			//char delimiter[] = ":";
			char delimiter = ':';
			char *ptr = strtok(recvRes, &delimiter);

			char *role = ptr;
			ptr = strtok(NULL, &delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, &delimiter);
			char *message = ptr;

			if (!strcmp(role, "p")) {
				if (!strcmp(topic, "shutDown")) {
					printf("\nPublisher %d disconnected.\n", argumentStructure.ordinalNumber);
					acceptedSockets[argumentStructure.clientNumber] = -1;
					break;
				}
				else {
					ptr = strtok(NULL, &delimiter);
					EnterCriticalSection(&message_queueAccess);
					Publish(message_queue, topic, message, argumentStructure.ordinalNumber);
					LeaveCriticalSection(&message_queueAccess);
					ReleaseSemaphore(pubSubSemaphore, 1, NULL);

				}
			}
			free(recvRes);
		}
		else if (!strcmp(recvRes, "ErrorS")) {
			break;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
			// connection was closed gracefully
			printf("\nConnection with client closed.\n");
			closesocket(argumentStructure.socket);
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			// there was an error during recv
			printf("\nrecv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentStructure.socket);
			break;

		}
	}
	free(recvRes);
	publisherThreadKilled = argumentStructure.ordinalNumber;
	return 1;
}


int  main(void)
{
	AddTopics(queue);

	InitializeCriticalSection(&queueAccess);
	InitializeCriticalSection(&message_queueAccess);

	pubSubSemaphore = CreateSemaphore(0, 0, 1, NULL);

	HANDLE pubSubThread;
	DWORD pubSubThreadID;

	HANDLE closeHandlesThread;
	DWORD closeHandlesThreadID;

	HANDLE exitThread;
	DWORD exitThreadID;

	
	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;
	// Socket used for communication with client
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
		printf("\ngetaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(AF_INET,      // IPv4 address famly
		SOCK_STREAM,  // stream socket
		IPPROTO_TCP); // TCP

	if (listenSocket == INVALID_SOCKET)
	{
		printf("\nsocket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address 
	// to socket
	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("\nbind failed with error: %d\n", WSAGetLastError());
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
		printf("\nioctlsocket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}
	//NEW

	// Since we don't need resultingAddress any more, free it
	freeaddrinfo(resultingAddress);

	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("\nlisten failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("\nServer initialized, waiting for clients.\n");

	pubSubThread = CreateThread(NULL, 0, &PubSubWork, NULL, 0, &pubSubThreadID);
	exitThread = CreateThread(NULL, 0, &GetChar, &listenSocket, 0, &exitThreadID);
	closeHandlesThread = CreateThread(NULL, 0, &CloseHandles, NULL, 0, &closeHandlesThreadID);


	while (clientsCount < NUMBER_OF_CLIENTS && serverRunning)
	{
		int selectResult = SelectFunction(listenSocket, 'r');
		if (selectResult == -1) {
			break;
		}

		acceptedSockets[clientsCount] = accept(listenSocket, NULL, NULL);

		if (acceptedSockets[clientsCount] == INVALID_SOCKET)
		{
			printf("\naccept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		char clientType = Connect(acceptedSockets[clientsCount]);
		if (clientType=='s') {
			SubscriberRecvThreads[numberOfConnectedSubs] = CreateThread(NULL, 0, &SubscriberReceive, &subscriberThreadArgument, 0, &SubscriberRecvThreadsID[numberOfConnectedSubs]);
			numberOfConnectedSubs++;
		}
		else {
			PublisherThreads[numberOfPublishers] = CreateThread(NULL, 0, &PublisherWork, &publisherThreadArgument, 0, &PublisherThreadsID[numberOfPublishers]);
			numberOfPublishers++;
		}
		 

		clientsCount++;

	} 

	for (int i = 0; i < numberOfPublishers; i++) {

		if (PublisherThreads[i])
			WaitForSingleObject(PublisherThreads[i], INFINITE);
	}

	for (int i = 0; i < numberOfConnectedSubs; i++) {

		if (SubscriberRecvThreads[i])
			WaitForSingleObject(SubscriberRecvThreads[i], INFINITE);
	}

	for (int i = 0; i < numberOfSubscribedSubs; i++) {

		if (SubscriberSendThreads[i])
			WaitForSingleObject(SubscriberSendThreads[i], INFINITE);
	}
	 
	if (pubSubThread) {
		WaitForSingleObject(pubSubThread, INFINITE);
	}

	if (exitThread) {
		WaitForSingleObject(exitThread, INFINITE);
	}

	if (closeHandlesThread) {
		WaitForSingleObject(closeHandlesThread, INFINITE);
	}

	printf("\nServer shutting down.\n");

	DeleteCriticalSection(&queueAccess);
	DeleteCriticalSection(&message_queueAccess);

	for (int i = 0; i < numberOfPublishers; i++) {
		SAFE_DELETE_HANDLE(PublisherThreads[i]);
	}

	for (int i = 0; i < numberOfConnectedSubs; i++) {
		SAFE_DELETE_HANDLE(SubscriberRecvThreads[i]);
	}

	for (int i = 0; i < numberOfSubscribedSubs; i++) {
		SAFE_DELETE_HANDLE(SubscriberSendThreads[i]);
	}

	SAFE_DELETE_HANDLE(pubSubThread);
	SAFE_DELETE_HANDLE(exitThread);
	SAFE_DELETE_HANDLE(closeHandlesThread);

	if (serverRunning) {
		// shutdown the connection since we're done
		for (int i = 0; i < NUMBER_OF_CLIENTS; i++) {

			iResult = shutdown(acceptedSockets[i], SD_SEND);
			if (iResult == SOCKET_ERROR)
			{
				printf("\nshutdown failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSockets[clientsCount]);
				WSACleanup();
				return 1;
			}
			closesocket(acceptedSockets[i]);
		}
		// cleanup
		closesocket(listenSocket);

	}
	
	free(queue->array);
	free(queue);
	free(message_queue->array);
	free(message_queue);

	WSACleanup();


	return 0;
}

