#ifndef PATH_FINDER
#define PATH_FINDER

#include <stdbool.h>

// a convenience macro for accessing a 2D point in a 1D array
#define grid_get_at(grid, cols, loc) \
grid[ loc.y * cols + loc.x ]

// a location that represents a non-existing cell
#define null_loc \
(Loc){-1, -1}

// Describes a point on a grid
typedef struct
{
    int x;
    int y;
} Loc;

// Represents the direction of the parent of a node
typedef enum 
{
    UNKNOWN    = 0, // for unexpanded nodes
    NONE       = 1, // only for end, as it has no parent
    UP         = 2,
    RIGHT      = 3,
    DOWN       = 4,
    LEFT       = 5,
    UP_RIGHT   = 6,
    DOWN_RIGHT = 7,
    DOWN_LEFT  = 8,
    UP_LEFT    = 9
} Parent_Direction;

// Represents the path from start to end
typedef struct 
{
    Loc *locs;
    float cost;
    int nb;
} Path;

// Represents a single cell in the grid
typedef struct 
{
    float cost;
    int nb_steps;
    int enqueued; // 1 based index, 0 means not enqueued
    Parent_Direction parent_dir;
    bool visited;
} Cell;

// Returns the shortest path from start to end, avoiding obstacles on the grid
Path shortest_path(const bool *grid, int cols, int rows, Loc start, Loc end);

// Applies a direction to a given location
// Retruns the resulting location
Loc next_loc(Loc loc, Parent_Direction direction);

// Returns true if location is within the grid, false otherwise
bool within_grid(Loc loc, int cols, int rows);

// Returns true if l1 is the same location as l2
bool locs_eq(Loc l1, Loc l2);

#endif
