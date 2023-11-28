#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "path_finder.h"

// a priority queue of Cell pointers. Implemented using a min-heap
typedef struct
{
    Cell **data;
    int size;
    int cap;
} Priority_Queue;

// initialize the Priority_Queue with 'cap' as the maximum capacity.
void init_queue(Priority_Queue *pq, int cap);

// adds the element to the Priority_Queue
void enqueue(Priority_Queue *q, Cell *cell);

// removes the front of the Priority_Queue and returns it
Cell *dequeue(Priority_Queue *q);

#endif
