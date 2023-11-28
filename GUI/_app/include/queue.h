#include "path_finder.h"

// a Queue of Cell pointers, 
typedef struct
{
    Cell **data;
    int cap;
    int size;
    int start;
} Queue;

// initilizes a queue with a maximum capcity
void init_queue(Queue *q, int cap);

// add a cell pointer to the back of the queue
void enqueue(Queue* q, Cell *cell);

// remove and return the front of the queue
Cell *dequeue(Queue* q);