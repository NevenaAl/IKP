
#include <winsock2.h>
#include <Windows.h>


#define DEFAULT_BUFLEN 512
#define NUM_OF_SUBS 20
#define TOPIC_LEN 15
#define MESSAGE_LEN 250

struct topic_sub {
	char* topic;
	SOCKET subs_array[NUM_OF_SUBS];
	int size;
};
struct topic_message {
	char topic[TOPIC_LEN];
	char message[MESSAGE_LEN];
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

struct Subscriber {
	SOCKET sendTo;
	HANDLE hSemaphore;
};


struct MessageStruct
{
	int header;
	char message[DEFAULT_BUFLEN - 4];

};