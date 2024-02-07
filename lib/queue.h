/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node Node_t;

typedef struct Node
{
    void *data;
    Node_t *next;
    Node_t *prev;
} Node_t;

typedef struct Queue
{
    Node_t *head;
    Node_t *tail;
    int size;
} Queue_t;

// Function prototypes
Queue_t *createQueue();
int isEmpty(Queue_t *queue);
int enqueue(Queue_t *queue, void *data, int sizeof_data);
int enqueueHead(Queue_t *queue, void *data, int sizeof_data);
Node_t *dequeue(Queue_t *queue);
Node_t *dequeueHead(Queue_t *queue);
Node_t *front(Queue_t *queue);
Node_t *rear(Queue_t *queue);
int removeFrameNode(Queue_t *queue, int frame_number);
int getSize(Queue_t *queue);
void deleteQueue(Queue_t *queue);