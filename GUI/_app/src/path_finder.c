#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "../include/path_finder.h"
#include "../include/priority_queue.h"

bool locs_eq(Loc l1, Loc l2)
{
    return l1.x == l2.x && l1.y == l2.y;
}

bool within_grid(Loc loc, int cols, int rows)
{
    return (!(loc.x < 0 || loc.x >= cols) && !(loc.y < 0 || loc.y >= rows));
}

Loc next_loc(Loc loc, Parent_Direction direction)
{
    switch(direction)
    {
        case UP:
            loc.y--;
            break;
        case RIGHT:
            loc.x++;
            break;
        case DOWN:
            loc.y++;
            break;
        case LEFT:
            loc.x--;
            break;
        case UP_RIGHT:
            loc.x++;
            loc.y--;
            break;
        case DOWN_RIGHT:
            loc.x++;
            loc.y++;
            break;
        case DOWN_LEFT:
            loc.x--;
            loc.y++;
            break;
        case UP_LEFT:
            loc.x--;
            loc.y--;
            break;
        default:
            break;
    }
    
    return loc;
}

// Turns a node pointer to a location in a 2D grid
static Loc node_ptr_to_loc(Node *node, int cols, Node *grid)
{
    int node_index   = node - grid;
    int node_index_x = node_index % cols;
    int node_index_y = node_index / cols;
    
    return (Loc){.x = node_index_x, .y = node_index_y};
}

// Enqueues in the given priority queue the adjacenet nodes to the current node
// Ignoring unpassable nodes, nodes that were already expanded, and nodes that are too expensive
static void enqueue_unvisited_passable_adjacents_if_cheaper(Node *current, int cols, int rows, bool *obstacle_grid, Node *node_grid, Loc start, Priority_Queue *unexpanded)
{
    Loc current_loc = node_ptr_to_loc(current, cols, node_grid);
    
    Loc up         = (Loc){.x = current_loc.x,     .y = current_loc.y - 1};
    Loc right      = (Loc){.x = current_loc.x + 1, .y = current_loc.y};
    Loc down       = (Loc){.x = current_loc.x,     .y = current_loc.y + 1};
    Loc left       = (Loc){.x = current_loc.x - 1, .y = current_loc.y};
    Loc up_right   = (Loc){.x = current_loc.x + 1, .y = current_loc.y - 1};
    Loc down_right = (Loc){.x = current_loc.x + 1, .y = current_loc.y + 1};
    Loc down_left  = (Loc){.x = current_loc.x - 1, .y = current_loc.y + 1};
    Loc up_left    = (Loc){.x = current_loc.x - 1, .y = current_loc.y - 1};
    
    // flags that can be represented by a single bit
    enum {
        CAN_UP         = 1,
        CAN_RIGHT      = 2,
        CAN_DOWN       = 4,
        CAN_LEFT       = 8,
        CAN_UP_RIGHT   = 16,
        CAN_DOWN_RIGHT = 32,
        CAN_DOWN_LEFT  = 64,
        CAN_UP_LEFT    = 128
    };
    
    // an 8 bit number where each bit represents if a direction is within the grid
    unsigned char possible_directions = 0;
    
    possible_directions |= (current_loc.y != 0)        << 0; // up
    possible_directions |= (current_loc.x != cols - 1) << 1; // right
    possible_directions |= (current_loc.y != rows - 1) << 2; // down
    possible_directions |= (current_loc.x != 0)        << 3; // left
    possible_directions |= ((possible_directions & CAN_UP)   && (possible_directions & CAN_RIGHT)) << 4; // up right
    possible_directions |= ((possible_directions & CAN_DOWN) && (possible_directions & CAN_RIGHT)) << 5; // down right
    possible_directions |= ((possible_directions & CAN_DOWN) && (possible_directions & CAN_LEFT))  << 6; // down left
    possible_directions |= ((possible_directions & CAN_UP)   && (possible_directions & CAN_LEFT))  << 7; // up left
    
    // an array of locations such that 'locs[i]' will be the location of the ith node to enqueue
    const Loc locs[8] = {up, right, down, left, up_right, down_right, down_left, up_left};
    
    // an array of directions such that 'opposite_dirs[dir]' will be the opposite of that direction
    // used to get the parent of a node after it went 'dir'
    const Parent_Direction opposite_dirs[8] = {DOWN, LEFT, UP, RIGHT, DOWN_LEFT, UP_LEFT, UP_RIGHT, DOWN_RIGHT};
    
    // an array of costs such that 'step_costs[0..3]' which is the non-diagonal adjacents will be 1
    // while 'step_costs[4..7]' which is the diagonal adjacenets will be sqrt of 2
    const float sqrt2 = sqrtf(2);
    const float step_costs[8] = {1, 1, 1, 1, sqrt2, sqrt2, sqrt2, sqrt2};
    
    for(int i = 0 ; i < 8 ; i++)
    {
        bool within_grid = (possible_directions & (1 << i));
        if(within_grid)
        {
            float step_cost = step_costs[i];
            bool passable = grid_get_at(obstacle_grid, cols, locs[i]);
            bool unvisited = !grid_get_at(node_grid, cols, locs[i]).visited;
            bool cheaper_than_old_cost_or_unknown = grid_get_at(node_grid, cols, locs[i]).parent_dir == UNKNOWN || grid_get_at(node_grid, cols, locs[i]).cost > current->cost + step_cost;
            bool cheaper_than_start = grid_get_at(node_grid, cols, start).parent_dir == UNKNOWN || grid_get_at(node_grid, cols, start).cost > current->cost + step_cost;
            if(passable && unvisited && cheaper_than_old_cost_or_unknown && cheaper_than_start)
            {
                // set the cost as the previous node cost + step_cost
                grid_get_at(node_grid, cols, locs[i]).cost = current->cost + step_cost;
                // set the new parent of the enqueued node
                grid_get_at(node_grid, cols, locs[i]).parent_dir = opposite_dirs[i];
                // set the number of steps it took to reach the node
                grid_get_at(node_grid, cols, locs[i]).nb_steps = current->nb_steps + 1;
                
                enqueue(unexpanded, &grid_get_at(node_grid, cols, locs[i]));
            }
        }
    }
}

Path *shortest_path(bool *obstacle_grid, int cols, int rows, Loc start, Loc end)
{
    // if the start/end is not passable, return NULL
    if(!grid_get_at(obstacle_grid, cols, end) || !grid_get_at(obstacle_grid, cols, start))
    {
        return NULL;
    }
    
    // allocate for the node grid, setting the costs to INFINITY and the parents to UNKNOWN
    Node *node_grid = (Node*) malloc(cols * rows * sizeof(Node));
    
    memset(node_grid, 0, cols * rows * sizeof(Node));
    
    // the cost from end to end is 0, and end has no NONE parent
    grid_get_at(node_grid, cols, end) = (Node){.parent_dir = NONE, .cost = 0, .visited = false, .nb_steps = 0};
    
    Priority_Queue unexpanded = init_queue(cols * rows);
    
    // enqueue the end to the priority queue
    enqueue(&unexpanded,&grid_get_at(node_grid, cols, end));
    
    while(unexpanded.size != 0)
    {
        Node *current = dequeue(&unexpanded);
        
        current->visited = true;
        enqueue_unvisited_passable_adjacents_if_cheaper(current, cols, rows, obstacle_grid, node_grid, start, &unexpanded);
    }
    
    // if the start point still has UNKNOWN parent, it means no path was found. Return NULL
    if(grid_get_at(node_grid, cols, start).parent_dir == UNKNOWN)
    {
        free(unexpanded.data);
        free(node_grid);
        return NULL;
    }
    
    // allocate for a path, which is just a cost with an array of directions
    Path *path = (Path*) malloc(sizeof(Path) + (sizeof(Parent_Direction) * grid_get_at(node_grid, cols, start).nb_steps));
    path->nb = 0;
    path->cost = grid_get_at(node_grid, cols, start).cost;
    
    // fill the path with the directions from start to end
    Loc current = start;
    while(!locs_eq(current, end))
    {
        path->dirs[path->nb++] = grid_get_at(node_grid, cols, current).parent_dir;
        current = next_loc(current, grid_get_at(node_grid, cols, current).parent_dir);
    }
    
    // cleanup
    free(unexpanded.data);
    free(node_grid);
    return path;
}
