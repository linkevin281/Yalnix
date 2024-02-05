/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 */

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef HARDWARE_H
#include <hardware.h>
#endif

Node_t *createNode(void *data, int sizeof_data)
{
    Node_t *node = (Node_t *)malloc(sizeof(Node_t));
    void *data_copy = malloc(sizeof_data);
    memcpy(data_copy, data, sizeof_data);
    node->data = data_copy;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

// Create a new queue, the head and tail are dummy nodes
Queue_t *createQueue()
{
    Queue_t *queue = (Queue_t *)malloc(sizeof(Queue_t));
    if (queue == NULL)
    {
        return NULL;
    }
    queue->head = createNode(NULL, 0);
    queue->tail = createNode(NULL, 0);
    queue->head->next = queue->tail;
    queue->tail->prev = queue->head;
    queue->size = 0;
    return queue;
}

// Check if size is 0, return 1 if true
int isEmpty(Queue_t *queue)
{
    return (queue->size == 0);
}

// Add item to queue from head, return -1 if failed
int enqueue(Queue_t *queue, void *data, int sizeof_data)
{
    Node_t *node = createNode(data, sizeof_data);
    if (node == NULL)
    {
        return -1;
    }

    node->next = queue->head->next;
    node->prev = queue->head;
    queue->head->next->prev = node;
    queue->head->next = node;

    queue->size = queue->size + 1;
    return 0;
}

int enqueueBack(Queue_t *queue, void *data, int sizeof_data)
{
    Node_t *node = createNode(data, sizeof_data);
    if (node == NULL)
    {
        return -1;
    }

    node->next = queue->tail;
    node->prev = queue->tail->prev;
    queue->tail->prev->next = node;
    queue->tail->prev = node;

    queue->size = queue->size + 1;
    return 0;
}

// Remove item from tail, return NULL if failed
Node_t *dequeue(Queue_t *queue)
{
    if (isEmpty(queue))
    {
        return NULL;
    }

    Node_t *node = queue->tail->prev;
    queue->tail->prev = node->prev;
    node->prev->next = queue->tail;

    queue->size = queue->size - 1;
    return node;
}

/**
 *
 * WARNING: This shit probably doesnt work. Or maybe we're good.
 */
int removeFrameNode(Queue_t *queue, int frame_number)
{
    Node_t *current = queue->head->next;
    while (current != queue->tail)
    {
        if (*(int *)current->data == frame_number) // this line is bad
        {
            current->prev->next = current->next;
            current->next->prev = current->prev;
            queue->size = queue->size - 1;
            return *(int *)(current->data);
        }
        current = current->next;
    }
    return -1;
}

// Function to get front of queue, returns NULL on fail
Node_t *peekFront(Queue_t *queue)
{
    if (isEmpty(queue))
    {
        return NULL;
    }
    return queue->head->next;
}

// Function to get rear of queue, returns NULL on fail
Node_t *peekRear(Queue_t *queue)
{
    if (isEmpty(queue))
    {
        return NULL;
    }
    return queue->tail->prev;
}

int getSize(Queue_t *queue)
{
    return queue->size;
}

void deleteQueue(Queue_t *queue)
{
    while (!isEmpty(queue))
    {
        Node_t *node = dequeue(queue);
        free(node->data);
        free(node);
    }
    free(queue->head);
    free(queue->tail);
    free(queue);
}
