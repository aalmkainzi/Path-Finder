#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../include/path_finder.h"
#include "../include/priority_queue.h"

// Returns true if l1 is the same location as l2
bool locs_eq(Loc l1, Loc l2)
{
    return l1.x == l2.x && l1.y == l2.y;
}

// Returns true if location is within the grid, false otherwise
bool within_grid(Loc loc, int cols, int rows)
{
    return (!(loc.x < 0 || loc.x >= cols) && !(loc.y < 0 || loc.y >= rows));
}

// Applies a direction to a given location
// Retruns the resulting location
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

// Turns a cell pointer to a location in a 2D grid
static Loc cell_ptr_to_loc(const Cell *cell, int cols, const Cell *grid)
{
    int cell_index   = cell - grid;
    int cell_index_x = cell_index % cols;
    int cell_index_y = cell_index / cols;
    
    return (Loc){.x = cell_index_x, .y = cell_index_y};
}

// Enqueues in the given queue the adjacenet cells to the current cell
// Ignoring unpassable cells and cells that were already visited and cells that are too expensive
static void enqueue_unvisited_passable_adjacents_if_cheaper(const Cell *current, int cols, int rows, const bool *obstacle_grid, Cell *cell_grid, Loc start, Priority_Queue *unexpanded)
{
    Loc current_loc = cell_ptr_to_loc(current, cols, cell_grid);
    
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
    
    // an array of locations such that 'locs[i]' will be the location of the ith cell to enqueue
    const Loc locs[8] = {up, right, down, left, up_right, down_right, down_left, up_left};
    
    // an array of directions such that 'opposite_dirs[dir]' will be the opposite of that direction
    // used to get the parent of a cell after it went 'dir'
    const Parent_Direction opposite_dirs[8] = {DOWN, LEFT, UP, RIGHT, DOWN_LEFT, UP_LEFT, UP_RIGHT, DOWN_RIGHT};
    
    const float sqrt2 = sqrtf(2);
    
    // a macro that takse a direction and enqueues the cell it leads to, only if the cell is passable and unvisited and not too expensive
    #define enqueue_adjacent(i)                                                           \
    do {                                                                                  \
        const int adj = i - 2;                                                            \
        bool within_grid = (possible_directions & (1 << adj));                            \
        if(within_grid)                                                                   \
        {                                                                                 \
            const float step_cost = i >= UP_RIGHT ? sqrt2 : 1;                            \
            bool passable = grid_get_at(obstacle_grid, cols, locs[adj]);                  \
            bool unvisited = !grid_get_at(cell_grid, cols, locs[adj]).visited;            \
            bool cheaper_than_old_cost_or_unknown =                                       \
            (grid_get_at(cell_grid, cols, locs[adj]).parent_dir == UNKNOWN ||             \
            grid_get_at(cell_grid, cols, locs[adj]).cost > current->cost + step_cost);    \
            bool cheaper_than_start =                                                     \
            (grid_get_at(cell_grid, cols, start).parent_dir == UNKNOWN                    \
            || grid_get_at(cell_grid, cols, start).cost > current->cost + step_cost);     \
            if(passable && unvisited && cheaper_than_old_cost_or_unknown && cheaper_than_start) \
            {                                                                             \
                /* set the cost as the previous cell cost + step_cost */                  \
                grid_get_at(cell_grid, cols, locs[adj]).cost = current->cost + step_cost; \
                /* set the new parent of the enqueued cell */                             \
                grid_get_at(cell_grid, cols, locs[adj]).parent_dir = opposite_dirs[adj];  \
                /* set the number of steps it took to reach the cell */                   \
                grid_get_at(cell_grid, cols, locs[adj]).nb_steps = current->nb_steps + 1; \
                                                                                          \
                enqueue(unexpanded, &grid_get_at(cell_grid, cols, locs[adj]));            \
            }                                                                             \
        }                                                                                 \
    } while(0)
    
    // enqueuing the passable and unvisited adjacents if cheaper
    enqueue_adjacent(UP);
    enqueue_adjacent(RIGHT);
    enqueue_adjacent(DOWN);
    enqueue_adjacent(LEFT);
    enqueue_adjacent(UP_RIGHT);
    enqueue_adjacent(DOWN_RIGHT);
    enqueue_adjacent(DOWN_LEFT);
    enqueue_adjacent(UP_LEFT);
    
    #undef enqueue_adjacent
}

Path shortest_path(const bool *obstacle_grid, int cols, int rows, Loc start, Loc end)
{
    // if the start/end is not passable, return NULL
    if(!grid_get_at(obstacle_grid, cols, end) || !grid_get_at(obstacle_grid, cols, start))
    {
        return (Path){0};
    }
    
    static Cell *cell_grid = NULL;
    static int old_rows = 0;
    static int old_cols = 0;
    
    // reallocate for the cell grid if it's not big enough
    if(old_rows < rows || old_cols < cols)
    {
        cell_grid = realloc(cell_grid, rows * cols * sizeof(Cell));
    }
    
    // setting the parents to UNKNOWN and enqueued to 0
    memset(cell_grid, 0, rows * cols * sizeof(Cell));
    
    old_rows = rows;
    old_cols = cols;
    
    // the cost from end to end is 0, and end has no NONE parent
    grid_get_at(cell_grid, cols, end).parent_dir = NONE;
    
    static Priority_Queue unexpanded = { 0 };
    
    init_queue(&unexpanded, rows * cols);
    
    // enqueue the end
    enqueue(&unexpanded, &grid_get_at(cell_grid, cols, end));
    
    // until the queue is emptied, keep dequeuing
    while(unexpanded.size != 0)
    {
        Cell *current = dequeue(&unexpanded);
        current->visited = true;
        
        enqueue_unvisited_passable_adjacents_if_cheaper(current, cols, rows, obstacle_grid, cell_grid, start, &unexpanded);
    }
    
    // if the start point still has UNKNOWN parent, it means no path was found. Return NULL
    if(grid_get_at(cell_grid, cols, start).parent_dir == UNKNOWN)
    {
        return (Path){0};
    }
    
    // allocate for a path, which is just a cost with an array of locations
    Path path = { 0 }; 
    path.locs = (Loc*) malloc(sizeof(Loc) * (grid_get_at(cell_grid, cols, start).nb_steps + 1));
    path.nb   = grid_get_at(cell_grid, cols, start).nb_steps + 1;
    path.cost = grid_get_at(cell_grid, cols, start).cost;
    
    // fill the path with the locations of the cells in the path from start to end
    Loc loc_iter = start;
    for(int i = 0 ; i < path.nb ; i++)
    {
        path.locs[i] = loc_iter;
        loc_iter = next_loc(loc_iter, grid_get_at(cell_grid, cols, loc_iter).parent_dir);
    }
    
    return path;
}
