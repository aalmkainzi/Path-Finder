#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <uchar.h>
#include <locale.h>
#include "../include/path_finder.h"

bool char_to_bool(char c);
int read_int(const char* prompt);
void print_path(Path *path, Loc start, Loc end, const bool *obstacles, int cols, int rows);
bool *read_grid(int rows, int cols, Loc *start, Loc *end);

int main()
{
    const int rows = read_int("Number of rows: ");
    const int cols = read_int("Number of cols: ");
    
    Loc start = {-1, -1};
    Loc end   = {-1, -1};
    
    // start and end are assigned as output parameters
    bool *grid = read_grid(rows, cols, &start, &end);
    
    Path *path = shortest_path(grid, cols, rows, start, end);
    
    print_path(path, start, end, grid, cols, rows);
    
    // cleanup
    free(grid);
    free(path);
}

// reads the obstacles grid and the start/end points
bool *read_grid(int rows, int cols, Loc *start, Loc *end)
{
    // these describe on what line the start/end point found
    int start_set_on_line = -1;
    int end_set_on_line   = -1;
    
    // the obstacles grid, true is passable, false is unpassable
    bool *grid = (bool*) calloc(rows * cols, sizeof(bool));
    
    // allocating cols + 2 for '\0' and one more char to check if user inputted a larger string than needed
    const int max_line_len = cols + 2;
    char *line = (char*) calloc(max_line_len, sizeof(char));
    
    puts("\nEnter the grid:\n");
    
    // goto this label if start/end wasn't set
    grid_input_loop:
    start_set_on_line = -1;
    end_set_on_line   = -1;
    puts("1 for passable\n0 for unpassable\nS for start point\nE for end point\n");
    for(int i = 0 ; i < rows ; i++)
    {
        printf("row #%d:\n", i+1);
        
        // fgets returns NULL if EOF was reached and no string was read
        if(fgets(line, max_line_len, stdin) == NULL)
        {
            fprintf(stderr, "\nReached End-Of-File without reading full grid\n");
            exit(1);
        }
        
        int entered_line_len = strlen(line);
        
        // the line could include a newline character from fgets, so it's removed
        if(line[entered_line_len-1] == '\n')
        {
            line[entered_line_len-1] = '\0';
            entered_line_len--;
        }
        
        // entered wrong number of characters
        if(entered_line_len != cols)
        {
            if(entered_line_len > cols)
            {
                printf("Expected %d values, got more.\nRe-Enter the row.\n", cols);
                
                // drain the stdin buffer
                // to not write to the line on the next iteration from the current line that's too big
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            }
            else
            {
                printf("Expected %d values, got %d.\nRe-Enter the row.\n", cols, entered_line_len);
            }
            
            i--;
            continue;
        }
        
        for(int j = 0 ; j < cols ; j++)
        {
            if(line[j] == '1' || line[j] == '0')
            {
                // turn '0' to false and '1' to true, then insert into grid
                grid[i * cols + j] = char_to_bool(line[j]);
            }
            else if((line[j] == 'S' || line[j] == 's') && start_set_on_line == -1)
            {
                // make start point passable on the grid
                grid[i * cols + j] = true;
                *start = (Loc){.x = j, .y = i};
                start_set_on_line = i;
            }
            else if((line[j] == 'E' || line[j] == 'e') && end_set_on_line == -1)
            {
                // make end point passable on the grid
                grid[i * cols + j] = true;
                *end = (Loc){.x = j, .y = i};
                end_set_on_line = i;
            }
            else
            {
                // found an unexpected character on the line. If start/end was entered on this line, reset them
                fprintf(stderr, "\nUnexpected character '%c'\nRe-Enter the row.\n", line[j]);
                if(start_set_on_line == i) start_set_on_line = -1;
                if(end_set_on_line   == i) end_set_on_line   = -1;
                i--;
                break;
            }
        }
    }
    
    // if start/end weren't set, redo the grid
    if(start_set_on_line == -1)
    {
        puts("\nStart point wasn't set.\nRe-Enter the grid\n");
        goto grid_input_loop;
    }
    if(end_set_on_line == -1)
    {
        puts("\nEnd point wasn't set.\nRe-Enter the grid\n");
        goto grid_input_loop;
    }
    
    free(line);
    return grid;
}

// returns true if c='1'
// false otherwise
bool char_to_bool(char c)
{
    return c == '1';
}

// displays a prompt and reads a positive int from stdin
int read_int(const char *prompt)
{
    int ret;
    while(1)
    {
        puts(prompt);
        
        // scanf returns the number of args successfully read, or -1 on failure
        int err = scanf("%d", &ret);
        
        // to skip the newline
        getchar();
        
        if(ret <= 0 || err != 1)
        {
            puts("Must be a positive number");
        }
        else
        {
            return ret;
        }
    }
}

// prints the obstacles grid and the path from start to end if found
void print_path(Path *path, Loc start, Loc end, const bool *obstacles, int cols, int rows)
{
    if(path == NULL)
    {
        puts("\nno path found");
        return;
    }
    
    // set the locale to utf8
    // making utf32 characters assignable to char* by taking 4 bytes
    setlocale(LC_ALL, "en_US.utf8");
    
    // each cell needs 2 chars:
    // one for the '|' to the right of it
    // one for the cell itself
    // +
    // each row needs 2 char:
    // starts with '|'
    // ends with a '\n'
    // +
    // 1 nul character to terminate the string
    const int grid_str_size = (cols * rows) * 2 + (2 * rows) + 1;
    char32_t *grid_str32 = (char32_t*) calloc(grid_str_size, sizeof(char32_t));
    const int row_len = 2 * cols + 2; // including the \n
    
    // this array is used to go from bool to cell, such that:
    // obstacles_chars[false] = '🞨'
    // obstacles_chars[true]  = ' '
    const char32_t obstacles_chars[2] = {U'🞨', U' '};
    
    // this loop sets the obstacles on the grid string
    for(int i = 0 ; i < rows ; i++)
    {
        // the leftmost '|' of each line
        grid_str32[i * row_len] = U'|';
        
        // the cell followed by a '|' to the right of it
        for(int j = 0 ; j < cols ; j++)
        {
            char32_t char_at_cell = obstacles_chars[grid_get_at(obstacles, cols, ((Loc){j, i}))];
            grid_str32[i * row_len + (j * 2) + 1] = char_at_cell;
            grid_str32[i * row_len + (j * 2) + 2] = U'|';
        }
        
        // a newline to end the row
        grid_str32[i * row_len + row_len - 1] = U'\n';
    }
    
    // set the start and end on the grid string
    grid_str32[start.y * row_len + (start.x * 2) + 1] = U'S';
    grid_str32[end.y   * row_len + (end.x   * 2) + 1] = U'E';
    
    Loc current;
    
    // ignore the first direction (start to second cell), as it won't be printed ('S' already there)
    if(path->nb > 0)
    {
        current = next_loc(start, path->dirs[0]);
    }
    
    // loop over the directions in the path, not including the first direction
    for(int i = 1 ; i < path->nb ; ++i)
    {
        // set an arrow pointing to its parent in each cell in path
        // then update the current location to the next location on the path
        switch(path->dirs[i])
        {
            case UP:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡑';
                current.y--;
                break;
            case UP_RIGHT:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡕';
                current.y--;
                current.x++;
                break;
            case RIGHT:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡒';
                current.x++;
                break;
            case DOWN_RIGHT:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡖';
                current.y++;
                current.x++;
                break;
            case DOWN:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡓';
                current.y++;
                break;
            case DOWN_LEFT:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡗';
                current.y++;
                current.x--;
                break;
            case LEFT:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡐';
                current.x--;
                break;
            case UP_LEFT:
                grid_str32[current.y * row_len + (current.x * 2) + 1] = U'🡔';
                current.y--;
                current.x--;
                break;
            default:
                fprintf(stderr, "Unexpected direction of value '%d'\n", path->dirs[i]);
                exit(1);
                break;
        }
    }
    
    // turn the utf32 string to a utf8 string so it can be printed
    char *grid_str = (char*) calloc(grid_str_size * MB_CUR_MAX, sizeof(char));
    char *temp = grid_str;
    mbstate_t state = {0};
    for(int i = 0 ; i < grid_str_size ; i++)
    {
        temp += c32rtomb(temp, grid_str32[i], &state);
    }
    
    printf("\ncost: %.2f\n", path->cost);
    printf("\n%s\n", grid_str);
    
    // cleanup
    free(grid_str32);
    free(grid_str);
}

