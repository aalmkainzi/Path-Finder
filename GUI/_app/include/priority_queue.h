#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "path_finder.h"

// a priority queue of Node pointers
typedef struct Priority_Queue {
    int size;
    Node **data;
} Priority_Queue;

// initialize the Priority_Queue with 'cap' as the maximum capacity.
Priority_Queue init_queue(int cap);

// adds the element to the Priority_Queue
void enqueue(Priority_Queue *q, Node *node);

// removes the front of the Priority_Queue and returns it
Node *dequeue(Priority_Queue *q);

#endif
