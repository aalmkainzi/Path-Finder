#include <limits.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/path_finder.h"
#define STB_DS_IMPLEMENTATION
#include "../libs/stb_ds.h"
#define RAYGUI_IMPLEMENTATION
#include "../libs/raygui.h"

// this struct describes a mouse click on a grid cell
typedef struct Cell_Click
{
    Loc loc;
    MouseButton mouse_button;
    bool held;
} Cell_Click;

// draws a grid given the top left point and the rows and columns
// returns the cell that was clicked
Cell_Click draw_grid(Vector2 topleft, int cols, int rows);

// grows/shrinks the obstacles grid according to the new rows/cols
void resize_obstacles(bool ***obstacles, int cols, int rows);

// returns a heap allocated 1D array from an array of bool arrays
bool *obstacles_2d_to_1d(bool **obstacles);

// clamps a float between 2 values
int iclampf(float f, int min, int max);

// a convinence macro used to clear the path (free and NULL it, set cost string to empty)
#define clear_path() \
do { \
    free(path); \
    path = NULL; \
    cost_str[0] = '\0'; \
} while(0)

// a convinence macro for selecting a mode
#define set_select(mode) \
do { \
    Loc *_locs[3] = {&(Loc){}, &start, &end}; \
    select_mode = mode; \
    *_locs[mode] = null_loc; \
    clear_path(); \
} while(0)

const int line_thickness = 2;
int cell_size = 128;
const int button_size = 64;
const int button_pad = 50;

// this enum represents whether the Start/End point selector is enabled
enum {
    NO_SELECT = 0,
    START = 1,
    END = 2
} select_mode = NO_SELECT;

int main()
{
    // start off with a 3x3 grid with no obstacles
    int rows = 3;
    int cols = 3;
    
    Loc start = {2, 2};
    Loc end   = {0, 0};
    
    // initialize the obstacles
    bool **obstacles = NULL;
    arrsetlen(obstacles, rows);
    for(int i = 0 ; i < rows ; i++)
    {
        obstacles[i] = NULL;
        arrsetlen(obstacles[i], cols);
        for(int j = 0 ; j < cols ; j++)
            obstacles[i][j] = true;
    }
    
    // the path describes the directions from start to end
    Path *path = NULL;
    
    // this string will be displayed to show the path cost
    // "Cost: " => 6
    // "%.2f"   => 13
    // '\0'     => 1
    char cost_str[6 + 13 + 1] = "";
    
    // the location of the last obstacle set, used to not set/unset the same cell when right click is held
    Loc last_obstacle_changed = null_loc;
    
    // set up the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Path Finder");
    SetWindowMinSize(1280, 720);
    SetTargetFPS(60);
    
    // represents the scroll in the grid scroll panel (unused)
    Vector2 scroll = { 0 };
    
    // represents what's inside the scroll panel, i.e its conent
    Rectangle scroll_panel_content;
    
    // represents the sub-rectangle inside scroll_panel_content that is in view (unused)
    Rectangle scroll_view = { 0 };
    
    // represents the bounds of the scroll panel
    Rectangle scroll_panel;
    
    // represents the bounds of the buttons panel
    Rectangle buttons_panel;
    
    // loading the style
    GuiLoadStyle("../../style_bluish.rgs");
    
    // loading the icons
    GuiLoadIcons("../../iconset.rgi", false);
    GuiSetIconScale(3);
    
    Font font_big = GuiGetFont();
    font_big.baseSize = 5;
    
    while(!WindowShouldClose())
    {
        // setting the bounds for the buttons panel, scales with window height
        buttons_panel = (Rectangle){
            .x = 0,
            .y = 0,
            .width  = 165,
            .height = GetScreenHeight()
        };
        
        // setting the bounds for the scroll panel, scales with window size
        scroll_panel = (Rectangle){
            .x = buttons_panel.width,
            .y = 0,
            .width  = GetScreenWidth() - buttons_panel.width,
            .height = GetScreenHeight(),
        };
        
        // setting the scroll panel content rectangle, its size depends on rows/cols
        scroll_panel_content = (Rectangle){
            .x = 0,
            .y = 0,
            .width  = cols * (cell_size + line_thickness) + line_thickness,
            .height = rows * (cell_size + line_thickness)
        };
        
        // setting the Start button bounds rectangle
        Rectangle s_button = {
            .x = (buttons_panel.width  / 2) - (button_size / 2),
            .y = (buttons_panel.height / 2) - (button_size / 2) - button_size,
            .width  = button_size,
            .height = button_size
        };
        
        // setting the End button bounds rectangle
        Rectangle e_button = {
            .x = s_button.x,
            .y = s_button.y + button_size + button_pad,
            .width  = button_size,
            .height = button_size
        };
        
        // setting the clear button bounds rectangle
        Rectangle x_button = {
            .x = s_button.x,
            .y = s_button.y - button_pad - button_size,
            .width  = button_size,
            .height = button_size
        };
        
        // setting the path button bounds rectangle
        Rectangle p_button = {
            .x = s_button.x,
            .y = x_button.y - button_pad - button_size,
            .width  = button_size,
            .height = button_size
        };
        
        // setting the cost text bounds rectangle
        Rectangle cost_label = {
            .x = (buttons_panel.width/2) - (MeasureTextEx(font_big, cost_str, font_big.baseSize, 1).x),
            .y = x_button.y - button_pad - 9,
            .width  = buttons_panel.width,
            .height = button_size
        };
        
        // setting the row spinner bounds rectangle
        Rectangle r_spinner = {
            .x = s_button.x,
            .y = e_button.y + button_size + button_pad,
            .width  = 83,
            .height = 45
        };
        
        // setting the column spinner bounds rectangle
        Rectangle c_spinner = {
            .x = s_button.x,
            .y = r_spinner.y + r_spinner.height + button_pad,
            .width  = r_spinner.width,
            .height = r_spinner.height
        };
        
        // change cell size if '-' or '=' are pressed
        if(IsKeyDown(KEY_MINUS) && cell_size >= 20)
            cell_size-=2;
        if(IsKeyDown(KEY_EQUAL) && cell_size <= 256)
            cell_size+=2;
        
        BeginDrawing();
        
        ClearBackground(WHITE);
        
        // drawing the scroll panel
        GuiScrollPanel(scroll_panel, NULL, scroll_panel_content, &scroll, &scroll_view);
        
        // this vector2 represents the top left point of the grid
        Vector2 grid_topleft = {
            scroll_panel.x + scroll.x,
            scroll_panel.y + scroll.y
        };
        
        // draw the grid and get the clicked cell
        Cell_Click clicked_cell = draw_grid(grid_topleft, cols, rows);
        
        // checks whether a cell is clicked
        bool cell_is_clicked = !locs_eq(clicked_cell.loc, null_loc);
        
        // checks whether the same cell being held, useful for obstacle placing
        bool holding_the_same_cell = cell_is_clicked && clicked_cell.held && locs_eq(clicked_cell.loc, last_obstacle_changed);
        
        // if a path exists, draw it on the grid
        if(path)
        {
            Loc current = start;
            for(int i = 0 ; i < path->nb ; current = next_loc(current, path->dirs[i]), i++)
            {
                // this rectangle represents the current cell in the path
                Rectangle path_cell_rect = {
                    .x = grid_topleft.x + line_thickness + (current.x * (cell_size + line_thickness)),
                    .y = grid_topleft.y + line_thickness + (current.y * (cell_size + line_thickness)),
                    .width  = cell_size,
                    .height = cell_size
                };
                
                // draw the path cell as GREEN
                DrawRectangleRec(path_cell_rect, GREEN);
            }
            
            // draw the End cell as green since it's not included in the previous loop
            Rectangle end_rect = {
                .x = grid_topleft.x + line_thickness + (end.x * (cell_size + line_thickness)),
                .y = grid_topleft.y + line_thickness + (end.y * (cell_size + line_thickness)),
                .width  = cell_size,
                .height = cell_size
            };
            DrawRectangleRec(end_rect, GREEN);
            
            // set the path cost to the cost string
            sprintf(cost_str, "Cost: %.2f", path->cost);
        }
        
        // draw the Start icon on the grid if within it
        if(within_grid(start, cols, rows))
            GuiDrawIcon(220,
                grid_topleft.x + line_thickness + (start.x * (cell_size + line_thickness)),
                grid_topleft.y + line_thickness + (start.y * (cell_size + line_thickness)),
                cell_size / 16,
                BLACK);
            
        // draw the end icon on the grid if within it
        if(within_grid(end, cols, rows))
            GuiDrawIcon(221,
                grid_topleft.x + line_thickness + (end.x * (cell_size + line_thickness)),
                grid_topleft.y + line_thickness + (end.y * (cell_size + line_thickness)),
                cell_size / 16,
                BLACK);
        
        GuiDrawRectangle(buttons_panel, 1, WHITE, WHITE);
        
        // setting the font for the cost string
        GuiSetFont(font_big);
        GuiLabel(cost_label, cost_str);
        
        // get whether any of the buttons was clicked
        bool find_clicked  = GuiButton(p_button, "#73#");
        bool clear_clicked = GuiButton(x_button, "#113#");
        bool start_clicked = GuiButton(s_button, "#220#");
        bool end_clicked   = GuiButton(e_button, "#221#");
        
        if(clear_clicked)
        {
            // if Start/End is selected, unselect
            select_mode = NO_SELECT;
            
            // remove all obstacles
            for(int i = 0 ; i < rows ; i++)
            {
                for(int j = 0 ; j < cols ; j++)
                {
                    obstacles[i][j] = true;
                }
            }
            
            // clear the path
            clear_path();
        }
        
        if(start_clicked)
        {
            set_select(START);
        }
        
        if(end_clicked)
        {
            set_select(END);
        }
        
        // if path button was clicked and the Start and End are set, execute the shortest_path algorithm
        if(find_clicked && within_grid(start, cols, rows) && within_grid(end, cols, rows))
        {
            select_mode = NO_SELECT;
            bool *obstacles1d = obstacles_2d_to_1d(obstacles);
            free(path);
            path = shortest_path(obstacles1d, cols, rows, start, end);
            free(obstacles1d);
            if(!path)
            {
                sprintf(cost_str, "No Path");
            }
        }
        
        // setting the font for rows/cols spinners
        GuiSetFont(GetFontDefault());
        
        // drawing the spinners and updating the rows/cols
        int old_rows = rows, old_cols = cols;
        GuiSpinner(r_spinner, "Rows  ", &rows, 1, INT_MAX, false);
        GuiSpinner(c_spinner, "Cols  ", &cols, 1, INT_MAX, false);
        
        // if mouse is hovering over spinner and scrolls up/down, the value should change
        int mousex = GetMouseX();
        int mousey = GetMouseY();
        
        float spinner_scroll = GetMouseWheelMoveV().y;
        if(mousex >= r_spinner.x && mousex <= r_spinner.x + r_spinner.width)
            if(mousey >= r_spinner.y && mousey <= r_spinner.y + r_spinner.height)
                rows += iclampf(spinner_scroll, -1, 1);
        
        if(mousex >= c_spinner.x && mousex <= c_spinner.x + c_spinner.width)
            if(mousey >= c_spinner.y && mousey <= c_spinner.y + c_spinner.height)
                cols += iclampf(spinner_scroll, -1, 1);
        
        // resize the obstacles grid if rows/cols was changed
        if(old_rows != rows || old_cols != cols)
        {
            resize_obstacles(&obstacles, cols, rows);
            clear_path();
        }
        
        // draw the obstacles
        for(int i = 0 ; i < rows ; i++)
        {
            for(int j = 0 ; j < cols ; j++)
            {
                if(!obstacles[i][j])
                {
                    Rectangle obstacle_rect = {
                        .x = grid_topleft.x + line_thickness + (j * (cell_size + line_thickness)),
                        .y = grid_topleft.y + line_thickness + (i * (cell_size + line_thickness)),
                        .width  = cell_size,
                        .height = cell_size
                    };
                    
                    DrawRectangleRec(obstacle_rect, RED);
                }
            }
        }
        
        // have the cursor be normal or an S or an E depending on the select mode
        const int cursor_icon_size = 4;
        switch(select_mode)
        {
            case NO_SELECT:
                ShowCursor();
                
                // since we're in normal cursor mode, the user should be able to click on the S/E on the grid
                if(cell_is_clicked && clicked_cell.mouse_button == MOUSE_BUTTON_LEFT)
                {
                    if(locs_eq(clicked_cell.loc, start) && !clicked_cell.held)
                    {
                        set_select(START);
                    }
                    if(locs_eq(clicked_cell.loc, end) && !clicked_cell.held)
                    {
                        set_select(END);
                    }
                }
                break;
            case START:
                // turn cursor into an S
                HideCursor();
                GuiDrawIcon(220, GetMouseX() - 2, GetMouseY() - (cursor_icon_size / 2) - (2 * cursor_icon_size), cursor_icon_size, BLACK);
                
                // since we're in S cursor mode, clicking on a passable cell will put the Start point there
                if(cell_is_clicked && clicked_cell.mouse_button == MOUSE_BUTTON_LEFT && !clicked_cell.held && obstacles[clicked_cell.loc.y][clicked_cell.loc.x])
                {
                    start.x = clicked_cell.loc.x;
                    start.y = clicked_cell.loc.y;
                    
                    // if the user puts the Start at the same spot that E was, remove E
                    if(locs_eq(start, end))
                        end = null_loc;
                    
                    // make cursor normal next frame, and clear path if it was drawn (since the grid was changed, the path might not apply anymore)
                    select_mode = NO_SELECT;
                    clear_path();
                }
                break;
            case END:
                // turn cursor into an E
                HideCursor();
                GuiDrawIcon(221, GetMouseX() - 2, GetMouseY() - (cursor_icon_size / 2) - (2 * cursor_icon_size), cursor_icon_size, BLACK);
                
                // since we're in E cursor mode, clicking on a passable cell will put the End point there
                if(cell_is_clicked && clicked_cell.mouse_button == MOUSE_BUTTON_LEFT && !clicked_cell.held && obstacles[clicked_cell.loc.y][clicked_cell.loc.x])
                {
                    end.x = clicked_cell.loc.x;
                    end.y = clicked_cell.loc.y;
                    
                    // if the user puts the End at the same spot that S was, remove S
                    if(locs_eq(start, end))
                        start = null_loc;
                    
                    // make cursor normal next frame, and clear path if it was drawn (since the grid was changed, the path might not apply anymore)
                    select_mode = NO_SELECT;
                    clear_path();
                }
                break;
        }
        
        // right click/hold will set/unset obstacles, as long as its not on Start/End
        if(cell_is_clicked && !holding_the_same_cell && clicked_cell.mouse_button == MOUSE_BUTTON_RIGHT && !locs_eq(clicked_cell.loc, start) && !locs_eq(clicked_cell.loc, end))
        {
            obstacles[clicked_cell.loc.y][clicked_cell.loc.x] ^= 1; // set/unset obstacle
            clear_path();
            last_obstacle_changed = clicked_cell.loc;
        }
        
        EndDrawing();
    }
    
    // cleanup
    for(int i = 0 ; i < arrlen(obstacles) ; i++)
    {
        arrfree(obstacles[i]);
    }
    arrfree(obstacles);
    
    free(path);
    CloseWindow();
}

Cell_Click draw_grid(Vector2 topleft, int cols, int rows)
{
    Cell_Click ret = {
        .loc  = null_loc,
        .held = false
    };
    
    // represents the bounds of the grid
    Rectangle grid_rect = {
        topleft.x,
        topleft.y,
        cols * (cell_size + line_thickness) + line_thickness,
        rows * (cell_size + line_thickness)
    };
    
    int mousex = GetMouseX();
    int mousey = GetMouseY();
    
    // the x and y of the cell that is currently hovered by the mouse cursor
    int cellx = (mousex - topleft.x) / (cell_size + line_thickness);
    int celly = (mousey - topleft.y) / (cell_size + line_thickness);
    
    // check if the hovered cell is within the grid
    if(cellx >= 0 && cellx < cols && mousex >= grid_rect.x)
    {
        if(celly >= 0 && celly < rows && mousey >= grid_rect.y)
        {
            // the bounds of the hovered_cell
            Rectangle hovered_cell = {
                .x = topleft.x + line_thickness + (cellx * (cell_size + line_thickness)),
                .y = topleft.y + line_thickness + (celly * (cell_size + line_thickness)),
                .width  = cell_size,
                .height = cell_size
            };
            
            // color the hovered cell with SKYBLUE
            DrawRectangleRec(hovered_cell, SKYBLUE);
            
            bool l_held    = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            bool l_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool r_held    = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
            bool r_pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
            
            // if left/right is held/pressed, assign the location of the cell, null_loc otherwise.
            // assign the mouse_button to the currently held/pressed mouse button.
            // if neither left nor right is pressed, then it was either held or nothing was pressed (in which case .loc is null_loc so it won't matter if .held is true).
            ret = (Cell_Click){
                .loc = l_held || l_pressed || r_held || r_pressed ? (Loc){cellx, celly} : null_loc,
                .mouse_button = (l_held || l_pressed) ? MOUSE_BUTTON_LEFT : MOUSE_BUTTON_RIGHT,
                .held = !l_pressed && !r_pressed
            };
        }
    }
    
    // drawing the rows
    for(int i = 0 ; i < rows + 1; i++)
    {
        Rectangle line = {topleft.x, topleft.y + (i * (cell_size + line_thickness)), cols * (cell_size + line_thickness) + line_thickness, line_thickness};
        DrawRectangleRec(line, BLACK);
    }
    
    // drawing the cols
    for(int i = 0 ; i < cols + 1; i++)
    {
        Rectangle line = {topleft.x + (i * (cell_size + line_thickness)), topleft.y, line_thickness, rows * (cell_size + line_thickness)};
        DrawRectangleRec(line, BLACK);
    }
    
    return ret;
}

// grows/shrinks the obstacles grid according to the new rows/cols
void resize_obstacles(bool ***obstacles, int cols, int rows)
{
    int old_rows = arrlen(*obstacles);
    int old_cols = arrlen((*obstacles)[0]);
    
    arrsetlen(*obstacles, rows);
    
    // in case new rows is bigger, make new rows
    for(int i = old_rows ; i < rows ; i++)
    {
        (*obstacles)[i] = NULL;
        arrsetlen((*obstacles)[i], cols);
        for(int j = 0 ; j < cols ; j++)
        {
            (*obstacles)[i][j] = true;
        }
    }
    
    for(int i = 0 ; i < rows ; i++)
    {
        arrsetlen((*obstacles)[i], cols);
        
        // in case new cols is bigger, make new cols
        for(int j = old_cols ; j < cols ; j++)
            (*obstacles)[i][j] = true;
    }
    
    // in case new rows is smaller, free old rows
    for(int j = rows ; j < old_rows ; j++)
    {
        arrfree((*obstacles)[j]);
        (*obstacles)[j] = NULL;
    }
}

// returns a heap allocated 1D array from an array of bool arrays
bool *obstacles_2d_to_1d(bool **obstacles)
{
    int rows = arrlen(obstacles);
    int cols = arrlen(obstacles[0]);
    
    bool *ret = calloc(rows * cols, sizeof(bool));
    for(int i = 0 ; i < rows ; i++)
    {
        for(int j = 0 ; j < cols ; j++)
        {
            ret[i * cols + j] = obstacles[i][j];
        }
    }
    
    return ret;
}

int iclampf(float f, int min, int max)
{
    return f > max ? max : (f < min ? min : f);
}
