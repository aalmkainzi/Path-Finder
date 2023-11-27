#include "../include/queue.h"
#include <stdlib.h>
#include <string.h>

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

void maybe_shift_queue(Queue *q)
{
    if(q->start + q->size >= q->cap)
    {
        memmove(q->data, q->data + q->start, q->size * sizeof(Cell*));
        q->start = 0;
    }
}

void enqueue(Queue* q, Cell *cell)
{
    maybe_shift_queue(q);
    q->data[q->start + q->size] = cell;
    q->size++;
}

Cell *dequeue(Queue* q)
{
    Cell *ret = q->data[q->start];
    q->size--;
    q->start++;
    return ret;
}


