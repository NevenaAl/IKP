#include "PubSub.h"

CRITICAL_SECTION queueAccess;
CRITICAL_SECTION message_queueAccess;

HANDLE pubSubSemaphore;


int clientsCount = 0;
SOCKET acceptedSockets[NUMBER_OF_CLIENTS];

struct Queue* queue = CreateQueue(10);
struct MessageQueue* message_queue = CreateMessageQueue(10);
struct topic_message current;
struct Subscriber subscribers[20];


HANDLE SubscriberThreads[NUMBER_OF_CLIENTS];
DWORD SubscriberThreadsID[NUMBER_OF_CLIENTS];

HANDLE PublisherThreads[NUMBER_OF_CLIENTS];
DWORD PublisherThreadsID[NUMBER_OF_CLIENTS];

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

				 iResult = shutdown(acceptedSockets[i], SD_BOTH);
				 if (iResult == SOCKET_ERROR)
				 {
					 printf("shutdown failed with error: %d\n", WSAGetLastError());
					 closesocket(acceptedSockets[i]);
					 WSACleanup();
					 return 1;
				 }
				 closesocket(acceptedSockets[i]);
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
	SOCKET sendSocket = *(SOCKET*)lpParam;
	while (serverRunning) {
		for (int i = 0; i < (sizeof(subscribers) / sizeof(Subscriber)); i++)
		{
			if (sendSocket == subscribers[i].sendTo) {
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
		int sendResult = SendFunction(sendSocket, (char*)messageStruct, messageSize);
		if (sendResult == -1)
			break;


		//free(message);
		//free(messageStruct);
	}
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
	ThreadArgument argumentStructure = *(ThreadArgument*)lpParam;
	bool subscriberRunning = true;
	memcpy(recvbuf,ReceiveFunction(argumentStructure.socket, recvbuf), DEFAULT_BUFLEN);
	if (strcmp(recvbuf, "ErrorC") && strcmp(recvbuf, "ErrorR") && strcmp(recvbuf, "ErrorS"))
	{
		char delimiter[] = ":";
		char *ptr = strtok(recvbuf, delimiter);

		char *role = ptr;
		ptr = strtok(NULL, delimiter);
		char *topic = ptr;
		ptr = strtok(NULL, delimiter);
		if (!strcmp(topic, "shutDown")) {
			SubscriberShutDown(queue, argumentStructure.socket, subscribers);
			subscriberRunning = false;
			return 0;
		}
		else {
			HANDLE hSem = CreateSemaphore(0, 0, 1, NULL);

			struct Subscriber subscriber;
			subscriber.sendTo = argumentStructure.socket;
			subscriber.hSemaphore = hSem;
			subscribers[argumentStructure.numberOfSubs] = subscriber;

			SubscriberThreads[argumentStructure.numberOfSubs] = CreateThread(NULL, 0, &SubscriberWork, &argumentStructure.socket, 0, &SubscriberThreadsID[argumentStructure.numberOfSubs]);

			EnterCriticalSection(&queueAccess);
			Subscribe(queue, argumentStructure.socket, topic);
			LeaveCriticalSection(&queueAccess);
			printf("Subscriber %d subscribed to topic: %s. \n", argumentStructure.numberOfSubs, topic);
		}
	}
	else if (!strcmp(recvbuf, "ErrorS")) {
		return 1;
	}
	else if (!strcmp(recvbuf, "ErrorC"))
		{
			printf("Connection with client closed.\n");
			closesocket(argumentStructure.socket);
	}
	else if (!strcmp(recvbuf, "ErrorR"))
	{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentStructure.socket);

	}

	while (subscriberRunning && serverRunning) {

		memcpy(recvbuf, ReceiveFunction(argumentStructure.socket, recvbuf), DEFAULT_BUFLEN);;
		if (strcmp(recvbuf, "ErrorC") && strcmp(recvbuf, "ErrorR") && strcmp(recvbuf, "ErrorS"))
		{
			char delimiter[] = ":";
			char *ptr = strtok(recvbuf, delimiter);

			char *role = ptr;
			ptr = strtok(NULL, delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			if (!strcmp(topic, "shutDown")) {
				SubscriberShutDown(queue, argumentStructure.socket, subscribers);
				subscriberRunning = false;
				break;
			}
			
			EnterCriticalSection(&queueAccess);
			Subscribe(queue, argumentStructure.socket, topic);
			LeaveCriticalSection(&queueAccess);
			printf("Subscriber %d subscribed to topic: %s. \n", argumentStructure.numberOfSubs, topic);

		}
		else if (!strcmp(recvbuf, "ErrorS")) {
			break;
		}
		else if (!strcmp(recvbuf, "ErrorC"))
		{
			// connection was closed gracefully
			printf("Connection with client closed.\n");
			closesocket(argumentStructure.socket);
			break;
		}
		else if (!strcmp(recvbuf, "ErrorR"))
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentStructure.socket);
			break;

		}
	}
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
	SOCKET recvSocket = *(SOCKET*)lpParam;

	while (serverRunning) {

		memcpy(recvbuf, ReceiveFunction(recvSocket, recvbuf), DEFAULT_BUFLEN);
		if (strcmp(recvbuf, "ErrorC") && strcmp(recvbuf, "ErrorR") && strcmp(recvbuf, "ErrorS"))
		{
			char delimiter[] = ":";
			char *ptr = strtok(recvbuf, delimiter);

			char *role = ptr;
			ptr = strtok(NULL, delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			char *message = ptr;

			if (!strcmp(role, "p")) {
				if (!strcmp(topic, "shutDown")) {
					break;
				}
				else {
					ptr = strtok(NULL, delimiter);
					EnterCriticalSection(&message_queueAccess);
					Publish(message_queue, topic, message);
					LeaveCriticalSection(&message_queueAccess);
					ReleaseSemaphore(pubSubSemaphore, 1, NULL);

				}
			}
		}
		else if (!strcmp(recvbuf, "ErrorS")) {
			break;
		}
		else if (!strcmp(recvbuf, "ErrorC"))
		{
			// connection was closed gracefully
			printf("Connection with client closed.\n");
			closesocket(recvSocket);
			break;
		}
		else if (!strcmp(recvbuf, "ErrorR"))
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(recvSocket);
			break;

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

	InitializeCriticalSection(&queueAccess);
	InitializeCriticalSection(&message_queueAccess);

	pubSubSemaphore = CreateSemaphore(0, 0, 1, NULL);

	HANDLE pubSubThread;
	DWORD pubSubThreadID;

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

	pubSubThread = CreateThread(NULL, 0, &PubSubWork, NULL, 0, &pubSubThreadID);
	exitThread = CreateThread(NULL, 0, &GetChar, &listenSocket, 0, &exitThreadID);


	//printf("Press X if you want to close server.\n");
	while (clientsCount < NUMBER_OF_CLIENTS && serverRunning)
	{
		int selectResult = SelectFunction(listenSocket, 'r');
		if (selectResult == -1) {
			break;
		}

		acceptedSockets[clientsCount] = accept(listenSocket, NULL, NULL);

		if (acceptedSockets[clientsCount] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		char clientType = Connect(acceptedSockets[clientsCount]);
		if (clientType=='s') {
			SubscriberThreads[numberOfSubscribers] = CreateThread(NULL, 0, &SubscriberReceive, &argumentStructure, 0, &SubscriberThreadsID[numberOfSubscribers]);
		}
		else {
			PublisherThreads[numberOfPublishers] = CreateThread(NULL, 0, &PublisherWork, &acceptedSockets[clientsCount], 0, &PublisherThreadsID[numberOfPublishers]);
		}
		 //   memcpy(recvbuf, ReceiveFunction(acceptedSockets[clientsCount], recvbuf), DEFAULT_BUFLEN);
			//if (strcmp(recvbuf,"ErrorC") && strcmp(recvbuf, "ErrorR"))
			//{
			//	char delimiter[] = ":";
			//	char *ptr = strtok(recvbuf, delimiter);

			//	char *role = ptr;
			//	ptr = strtok(NULL, delimiter);

			//	if (!strcmp(role, "s")) {
			//		ThreadArgument argumentStructure;
			//		argumentStructure.numberOfSubs = numberOfSubscribers;
			//		argumentStructure.socket = acceptedSockets[clientsCount];
			//		SubscriberThreads[numberOfSubscribers] = CreateThread(NULL, 0, &SubscriberReceive, &argumentStructure, 0, &SubscriberThreadsID[numberOfSubscribers]);
			//		printf("Subscriber %d connected.\n", numberOfSubscribers);
			//		numberOfSubscribers++;

			//	}

			//	if (!strcmp(role, "p")) {

			//		PublisherThreads[numberOfPublishers] = CreateThread(NULL, 0, &PublisherWork, &acceptedSockets[clientsCount], 0, &PublisherThreadsID[numberOfPublishers]);
			//		printf("Publisher %d connected.\n",numberOfPublishers);
			//		numberOfPublishers++;
			//	}
			//}
			//else if (!strcmp(recvbuf, "ErrorC"))
			//{
			//	// connection was closed gracefully
			//	printf("Connection with client closed.\n");
			//	closesocket(acceptedSockets[clientsCount]);
			//}
			//else if (!strcmp(recvbuf, "ErrorR"))
			//{
			//	// there was an error during recv
			//	printf("recv failed with error: %d\n", WSAGetLastError());
			//	closesocket(acceptedSockets[clientsCount]);
			//}
		

		clientsCount++;

	} 

	for (int i = 0; i < numberOfPublishers; i++) {

		if (PublisherThreads[i])
			WaitForSingleObject(PublisherThreads[i], INFINITE);
	}

	for (int i = 0; i < numberOfSubscribers; i++) {

		if (SubscriberThreads[i])
			WaitForSingleObject(SubscriberThreads[i], INFINITE);
	}
	 
	if (pubSubThread) {
		WaitForSingleObject(pubSubThread, INFINITE);
	}

	if (exitThread) {
		WaitForSingleObject(exitThread, INFINITE);
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

	SAFE_DELETE_HANDLE(pubSubThread);
	SAFE_DELETE_HANDLE(exitThread);


	if (serverRunning) {
		// shutdown the connection since we're done
		for (int i = 0; i < NUMBER_OF_CLIENTS; i++) {

			iResult = shutdown(acceptedSockets[i], SD_SEND);
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

	}
	
	free(queue);
	free(message_queue);

	WSACleanup();


	return 0;
}

