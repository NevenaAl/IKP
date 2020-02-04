
#include <winsock2.h>
#include <Windows.h>


#define DEFAULT_BUFLEN 512
#define NUM_OF_SUBS 20
#define TOPIC_LEN 15
#define MESSAGE_LEN 250

struct TopicSubscribers {
	char topic[TOPIC_LEN];
	SOCKET subsArray[NUM_OF_SUBS];
	int size;
};

struct TopicMessage {
	char topic[TOPIC_LEN];
	char message[MESSAGE_LEN];
};

struct Queue
{
	int front, rear, size;
	unsigned capacity;
	TopicSubscribers* array;
};

struct MessageQueue
{
	int front, rear, size;
	unsigned capacity;
	TopicMessage* array;
};

struct Subscriber {
	SOCKET socket;
	HANDLE hSemaphore;
	bool running;
};

struct MessageStruct
{
	int header;
	char message[DEFAULT_BUFLEN - 4];

};

struct ThreadArgument {
	SOCKET socket;
	int ordinalNumber;
	int clientNumber;
};