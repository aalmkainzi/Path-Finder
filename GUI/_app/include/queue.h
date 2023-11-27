#include "path_finder.h"

typedef struct
{
    Cell **data;
    int cap;
    int size;
    int start;
} Queue;

Queue init_queue(int cap);

void enqueue(Queue* q, Cell *cell);

Cell *dequeue(Queue* q);