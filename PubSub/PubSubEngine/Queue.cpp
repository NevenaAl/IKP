#include "Queue.h"

struct Queue* CreateQueue(unsigned capacity)
{
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;  // This is important, see the Enqueue 
	queue->array = (topic_sub*)malloc(queue->capacity * sizeof(topic_sub));
	return queue;
}
struct MessageQueue* CreateMessageQueue(unsigned capacity)
{
	struct MessageQueue* queue = (struct MessageQueue*) malloc(sizeof(struct MessageQueue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;  // This is important, see the enqueue 
	queue->array = (topic_message*)malloc(queue->capacity * sizeof(topic_message));
	return queue;
}

// Queue is full when size becomes equal to the capacity  
int IsFull(struct Queue* queue)
{
	return (queue->size == queue->capacity);
}

// Queue is empty when size is 0 
int IsEmpty(struct Queue* queue)
{
	return (queue->size == 0);
}

// Function to add an item to the queue.   
// It changes rear and size 
void Enqueue(struct Queue* queue, char* topic)
{
	topic_sub item;
	item.topic = topic;
	item.subs_array = (SOCKET*)malloc(300);
	item.size = 0;

	if (IsFull(queue))
		ExpandQueue(queue);
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
	printf("%s Enqueued to queue\n", item.topic);
}

topic_sub Dequeue(struct Queue* queue)
{
	//if (isEmpty(queue))
//		return;
	topic_sub item = queue->array[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size = queue->size - 1;
	return item;
}

void ExpandQueue(struct Queue* queue) {
	queue->array = (topic_sub*)realloc(queue->array, queue->size + queue->capacity);
	queue->capacity += queue->capacity;
}
void ExpandMessageQueue(struct MessageQueue* queue) {
	queue->array = (topic_message*)realloc(queue->array, queue->size + queue->capacity);
	queue->capacity += queue->capacity;
}