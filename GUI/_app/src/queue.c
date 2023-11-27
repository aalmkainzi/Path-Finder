#include "../include/queue.h"
#include <stdlib.h>
#include <string.h>

// initilizes a queue with a maximum capcity
Queue init_queue(int cap)
{
    Queue ret = {
        .data = (Cell**) calloc(cap, sizeof(Cell*)),
        .start = 0,
        .size  = 0,
        .cap = cap
    };
    
    return ret;
}

// shift the queue back if capacity reached
static void maybe_shift_queue(Queue *q)
{
    if(q->start + q->size >= q->cap)
    {
        memmove(q->data, q->data + q->start, q->size * sizeof(Cell*));
        q->start = 0;
    }
}

// add a cell pointer to the back of the queue
void enqueue(Queue* q, Cell *cell)
{
    maybe_shift_queue(q);
    q->data[q->start + q->size] = cell;
    q->size++;
}

// remove and return the front of the queue
Cell *dequeue(Queue* q)
{
    Cell *ret = q->data[q->start];
    q->size--;
    q->start++;
    return ret;
}