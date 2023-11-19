#include <stdlib.h>
#include "../include/priority_queue.h"

#define parent(n) ((n-1)/2)
#define left(n)   (2*n + 1)
#define right(n)  (2*n + 2)
#define root      (0)

Priority_Queue init_queue(int cap)
{
    Priority_Queue ret = {
        .data = (Node**) calloc(cap, sizeof(Node*))
    };
    
    return ret;
}

static void swap_nodes(Node **a, Node **b)
{
    Node *temp = *a;
    *a = *b;
    *b = temp;
}

// swaps the rightmost node with its parent iteratively until data structure is a proper min-heap
static void sift_up(Priority_Queue *q)
{
    int current = q->size - 1;
    
    while(current != 0 && q->data[current]->cost < q->data[parent(current)]->cost)
    {
        swap_nodes(&q->data[current], &q->data[parent(current)]);
        current = parent(current);
    }
}

// compares parent with left and right children
// returns the index of the smallest
static int min_of_family(Node **arr, int size, int parent)
{
    // parent has no children
    if(left(parent) >= size) 
    {
        return parent;
    }
    // parent only has left child
    else if(right(parent) >= size)
    {
        if(arr[left(parent)]->cost < arr[parent]->cost)
            return left(parent);
        return parent;
    }
    // parent has both children
    else
    {
        if(arr[left(parent)]->cost < arr[parent]->cost && arr[left(parent)]->cost <= arr[right(parent)]->cost)
            return left(parent);
        
        if(arr[right(parent)]->cost < arr[parent]->cost && arr[right(parent)]->cost <= arr[left(parent)]->cost)
            return right(parent);
        
        return parent;
    }
}

// swaps parent with least of children iteratively until the data structure is a proper min-heap
static void sift_down(Priority_Queue *q)
{
    int parent = root;
    int old_parent;
    int least;
    
    do
    {
        least = min_of_family(q->data, q->size, parent);
        swap_nodes(&q->data[parent], &q->data[least]);
        old_parent = parent;
        parent = least;
    } while(least != old_parent);
}

void enqueue(Priority_Queue *q, Node *n)
{
    q->data[q->size] = n;
    q->size++;
    sift_up(q);
}

Node *dequeue(Priority_Queue *q)
{
    Node *ret = q->data[0];
    q->data[0] = q->data[q->size - 1];
    q->size--;
    
    sift_down(q);
    return ret;
}
