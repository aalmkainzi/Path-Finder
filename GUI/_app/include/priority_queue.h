#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "path_finder.h"

// a priority queue of Cell pointers
typedef struct
{
    int size;
    Cell **data;
} Priority_Queue;

// initialize the Priority_Queue with 'cap' as the maximum capacity.
Priority_Queue init_queue(int cap);

// adds the element to the Priority_Queue
void enqueue(Priority_Queue *q, Cell *cell);

// removes the front of the Priority_Queue and returns it
Cell *dequeue(Priority_Queue *q);

#endif
