
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define SERVER_SLEEP_TIME 50
#define NUMBER_OF_CLIENTS 20
#define SAFE_DELETE_HANDLE(h) {if(h)CloseHandle(h);}

bool InitializeWindowsSockets();
void SelectFunc(int, SOCKET, char);
void Subscribe(struct Queue*, SOCKET, char*);
void Publish(struct MessageQueue*, char*, char*);
void ExpandQueue(struct Queue*); //da li je potrebno
void ExpandMessageQueue(struct MessageQueue*);

struct topic_sub {
	char* topic;
	SOCKET* subs_array;
	int size;
};
struct topic_message {
	char* topic;
	char* message;
};
struct Queue
{
	int front, rear, size;
	unsigned capacity;
	topic_sub* array;
};
struct MessageQueue
{
	int front, rear, size;
	unsigned capacity;
	topic_message* array;
};

// function to create a queue of given capacity.  
// It initializes size of queue as 0 
struct Queue* createQueue(unsigned capacity)
{
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;  // This is important, see the enqueue 
	queue->array = (topic_sub*)malloc(queue->capacity * sizeof(topic_sub));
	return queue;
}
struct MessageQueue* createMessageQueue(unsigned capacity)
{
	struct MessageQueue* queue = (struct MessageQueue*) malloc(sizeof(struct MessageQueue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;  // This is important, see the enqueue 
	queue->array = (topic_message*)malloc(queue->capacity * sizeof(topic_message));
	return queue;
}

// Queue is full when size becomes equal to the capacity  
int isFull(struct Queue* queue)
{
	return (queue->size == queue->capacity);
}

// Queue is empty when size is 0 
int isEmpty(struct Queue* queue)
{
	return (queue->size == 0);
}

// Function to add an item to the queue.   
// It changes rear and size 
void enqueue(struct Queue* queue, char* topic)
{
	topic_sub item;
	item.topic = topic;
	item.subs_array = (SOCKET*)malloc(300);
	item.size = 0;

	if (isFull(queue))
	   ExpandQueue(queue);
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
	printf("%s enqueued to queue\n", item.topic);
}

topic_sub dequeue(struct Queue* queue)
{
	//if (isEmpty(queue))
//		return;
	topic_sub item = queue->array[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size = queue->size - 1;
	return item;
}
DWORD WINAPI PublisherWork(LPVOID lpParam) 
{
	//u ovom threadu cekati na recv od publisher klijenta
	return 0;
}
DWORD WINAPI SubscriberWork(LPVOID lpParam)
{
	//u ovom threadu send pa subscirber klijentu kad publisher nesto stavi u message queue
	return 0;
}
int  main(void)
{
	struct Queue* queue = createQueue(1000);
	struct MessageQueue* message_queue = createMessageQueue(1000);
	
	enqueue(queue, "Sport");
	enqueue(queue, "Fashion");

	int clientsCount = 0;

	HANDLE ClientThreads[NUMBER_OF_CLIENTS];
	DWORD ClientThreadsID[NUMBER_OF_CLIENTS];
	
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
		// Wait for clients and accept client connections.
		// Returning value is acceptedSocket used for further
		// Client<->Server communication. This version of
		// server will handle only one client.
		//NEW
		SelectFunc(iResult, listenSocket, 'r');

		acceptedSockets[clientsCount] = accept(listenSocket, NULL, NULL);

		if (acceptedSockets[clientsCount] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}


		do
		{
			unsigned long int nonBlockingMode = 1;
			iResult = ioctlsocket(acceptedSockets[clientsCount], FIONBIO, &nonBlockingMode);

			if (iResult == SOCKET_ERROR)
			{
				printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
				return 1;
			}
			SelectFunc(iResult, acceptedSockets[clientsCount], 'r');

			// Receive data until the client shuts down the connection

			iResult = recv(acceptedSockets[clientsCount], recvbuf, DEFAULT_BUFLEN, 0);

			char delimiter[] = ":";
			char *ptr = strtok(recvbuf, delimiter);

			char *role = ptr;
			ptr = strtok(NULL, delimiter);
			char *topic = ptr;
			ptr = strtok(NULL, delimiter);
			char *message = ptr;
			ptr = strtok(NULL, delimiter);

			if (!strcmp(role, "s")) {

				ClientThreads[clientsCount] = CreateThread(NULL, 0, &SubscriberWork, &acceptedSockets[clientsCount], 0, &ClientThreadsID[clientsCount]);
				Subscribe(queue, acceptedSockets[clientsCount], topic);
			}

			if (!strcmp(role, "p")) {

				ClientThreads[clientsCount] = CreateThread(NULL, 0, &PublisherWork, &acceptedSockets[clientsCount], 0, &ClientThreadsID[clientsCount]);
				Publish(message_queue,topic,message);
			}

			if (iResult > 0)
			{
				if (!strcmp(role, "p")) {
					printf("Publisher published message: %s to topic: %s. \n",message, topic);
				}
				if (!strcmp(role, "s")) {
					printf("Subscriber subscribed to topic: %s. \n", topic);
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
		} while (iResult > 0);

		// here is where server shutdown loguc could be placed
		clientsCount++;

	} while (clientsCount < NUMBER_OF_CLIENTS);

	// shutdown the connection since we're done
	iResult = shutdown(acceptedSockets[clientsCount], SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSockets[clientsCount]);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(listenSocket);
	closesocket(acceptedSockets[clientsCount]);
	WSACleanup();

	return 0;
}
void ExpandQueue(struct Queue* queue) {
	queue->array = (topic_sub*)realloc(queue->array,queue->size + queue->capacity);
	queue->capacity += queue->capacity;
}
void ExpandMessageQueue(struct MessageQueue* queue) {
	queue->array = (topic_message*)realloc(queue->array, queue->size + queue->capacity);
	queue->capacity += queue->capacity;
}
void Subscribe(struct Queue* queue,SOCKET sub, char* topic) {
	for (int i = 0; i < queue->size; i++) {
		if (!strcmp(queue->array[i].topic,topic)) {
			int index = queue->array[i].size;
			queue->array[i].subs_array[index] = sub;
			queue->array[i].size++;
		}
	}
}

void Publish(struct MessageQueue* message_queue, char* topic, char* message) {
	//enqueue to message_queue
	topic_message item;
	item.topic = topic;
	item.message = message;

	if (message_queue->size == message_queue->capacity)
		ExpandMessageQueue(message_queue);
	message_queue->rear = (message_queue->rear + 1) % message_queue->capacity;
	message_queue->array[message_queue->rear] = item;
	message_queue->size = message_queue->size + 1;
	
}
void SelectFunc(int iResult, SOCKET listenSocket, char rw) {
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
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

