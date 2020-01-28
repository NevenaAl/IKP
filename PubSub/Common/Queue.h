#pragma once
#include "winsock2.h"
#include <stdio.h>
#include "Structures.h"

///<summary>
/// Expanding queue if IsFull. 
///</summary>
///<param name ="queue"> Queue to expand.</param>
///<returns> No return value.</returns>
void ExpandQueue(struct Queue* queue) {
	queue->array = (topic_sub*)realloc(queue->array, queue->size * (sizeof(topic_sub)) + sizeof(topic_sub));
	queue->capacity += 1;
}

///<summary>
/// Expanding message queue if IsFullMessageQueue. 
///</summary>
///<param name ="queue"> Queue to expand.</param>
///<returns> No return value.</returns>
void ExpandMessageQueue(struct MessageQueue* queue) {
	queue->array = (topic_message*)realloc(queue->array, queue->size*(sizeof(topic_message)) + sizeof(topic_message));
	queue->capacity += 1;
}

///<summary>
/// Creating Queue. 
///</summary>
///<param name ="capacity"> Capacity of queue.</param>
///<returns> Created queue.</returns>
struct Queue* CreateQueue(unsigned capacity)
{
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;  // This is important, see the Enqueue 
	queue->array = (topic_sub*)malloc(queue->capacity * sizeof(topic_sub));
	return queue;
}

///<summary>
/// Creating Message Queue. 
///</summary>
///<param name ="capacity"> Capacity of queue.</param>
///<returns> Created message queue.</returns>
struct MessageQueue* CreateMessageQueue(unsigned capacity)
{
	struct MessageQueue* queue = (struct MessageQueue*) malloc(sizeof(struct MessageQueue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	queue->rear = capacity - 1;  // This is important, see the enqueue 
	queue->array = (topic_message*)malloc(queue->capacity * sizeof(topic_message));
	return queue;
}

///<summary>
/// Checking if size of queue is equal to it's capacity. 
///</summary>
///<param name ="queue"> Queue to check.</param>
///<returns> Indicates if queue is full.</returns> 
int IsFull(struct Queue* queue)
{
	return (queue->size == queue->capacity);
}

///<summary>
/// Checking if size of queue is equal to zero. 
///</summary>
///<param name ="queue"> Queue to check.</param>
///<returns> Indicates if queue is empty.</returns> 
int IsEmpty(struct Queue* queue)
{
	return (queue->size == 0);
}

///<summary>
/// Checking if size of message queue is equal to it's capacity. 
///</summary>
///<param name ="queue"> Queue to check.</param>
///<returns> Indicates if message queue is full..</returns>
int IsFullMessageQueue(struct MessageQueue* queue)
{
	return (queue->size == queue->capacity);
}

///<summary>
/// Checking if size of message queue is equal to zero. 
///</summary>
///<param name ="queue"> Queue to check.</param>
///<returns> Indicates if message queue is empty.</returns> 
int IsEmptyMessageQueue(struct MessageQueue*  queue)
{
	return (queue->size == 0);
}

///<summary>
/// Adding element to queue. 
///</summary>
///<param name ="queue"> Queue to add to.</param>
///<param name ="topic"> Topic to add.</param>
///<returns> No return value.</returns> 
void Enqueue(struct Queue* queue, char* topic)
{
	topic_sub item;
	//item.topic = topic;
	strcpy(item.topic, topic);
	//item.subs_array;
	item.size = 0;

	if (IsFull(queue))
		ExpandQueue(queue);
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
	printf("%s Enqueued to queue\n", item.topic);
}

///<summary>
/// Adding element to message queue. 
///</summary>
///<param name ="queue"> Queue to add to.</param>
///<param name ="topic"> Topic message structure to add.</param>
///<returns> No return value.</returns> 
void EnqueueMessageQueue(struct MessageQueue* queue, topic_message topic)
{
	if (IsFullMessageQueue(queue))
		ExpandMessageQueue(queue);
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = topic;
	queue->size = queue->size + 1;
	//printf("%s Enqueued to queue\n", topic.topic);
}

///<summary>
/// Deleting (poping) element from queue. 
///</summary>
///<param name ="queue"> Queue to delete from.</param>
///<returns> Poped element.</returns> 
topic_sub Dequeue(struct Queue* queue)
{
	if (!IsEmpty(queue)) {
		topic_sub item = queue->array[queue->front];
		queue->front = (queue->front + 1) % queue->capacity;
		queue->size = queue->size - 1;
		return item;
	}
}

///<summary>
/// Deleting (poping) element from message queue. 
///</summary>
///<param name ="queue"> Queue to delete from.</param>
///<returns> Poped element.</returns> 
topic_message DequeueMessageQueue(struct MessageQueue* queue)
{
	if (!IsEmptyMessageQueue(queue)) {
		topic_message item = queue->array[queue->front];
		queue->front = (queue->front + 1) % queue->capacity;
		queue->size = queue->size - 1;
		return item;
	}
}
