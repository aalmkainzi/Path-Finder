#include <limits.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "../include/path_finder.h"
#define STB_DS_IMPLEMENTATION
#include "../libs/stb_ds.h"
#define RAYGUI_IMPLEMENTATION
#include "../libs/raygui.h"

// this struct describes a mouse click on a grid cell
typedef struct
{
    Loc loc;
    MouseButton mouse_button;
    bool held;
} Cell_Click;

// draws a grid given the top left point and the rows and columns
// returns the cell that was clicked
Cell_Click draw_grid(Vector2 topleft, int cols, int rows, Rectangle view, int panel_borderx);

// grows/shrinks the obstacles grid according to the new rows/cols
void resize_obstacles(bool ***obstacles, int cols, int rows);

// returns a heap allocated 1D array from an array of bool arrays
bool *obstacles_2d_to_1d(bool **obstacles);

// scrolls the grid if dragging it with mouse
void scroll_by_dragging_mouse(Cell_Click cell_click, Vector2 *scroll);

// calls the shortest path algorithm and sets the path
double set_path(Path *path, bool **obstacles, int cols, int rows, Loc start, Loc end);

// draws the path as green squares on the grid, storing the path cells in the `path_cells`
void draw_path_and_set_cells(Path path, char *cost_str, Vector2 topleft);

// draws the row/col spinners and updates the rows and cols
bool draw_spinners_and_update_rows_cols(Rectangle r_spinner, Rectangle c_spinner, int *rows, int *cols);

// draws the obstacles on the grid as red squares
void draw_obstacles(bool **obstacles, int cols, int rows, Vector2 topleft, Rectangle view);

// removes all the obstacles from the grid
void clear_obstacles(bool ***obstacles, int cols, int rows);

// clamps a float between 2 int values
int iclampf(float f, int min, int max);

// a convinence macro used to clear the path (free and NULL it, set cost string to empty)
#define clear_path() \
do { \
    free(path.locs); \
    path = (Path){0}; \
    pop_up_open = false; \
    cost_str[0] = '\0'; \
} while(0)

// a convinence macro for setting a select mode
#define set_select_mode(mode) \
do { \
    _Static_assert(mode == START || mode == END, "set_select_mode must be either START or END"); \
    HideCursor(); \
    Loc *_locs[2] = {&start, &end}; \
    select_mode = mode; \
    *_locs[mode - 1] = null_loc; \
    clear_path(); \
} while(0)

// a convinence macro for unselecting
#define no_select() \
do { \
    select_mode = NO_SELECT; \
    ShowCursor(); \
} while(0);

const int line_thickness = 2;
int cell_size = 128;
const int button_size = 64;
const int button_pad = 50;

// this enum represents whether the Start/End point selector is enabled
enum
{
    NO_SELECT = 0,
    START = 1,
    END = 2
} select_mode = NO_SELECT;

#define INITIAL_ROWS 3
#define INITIAL_COLS 3

int main()
{
    // start off with a 3x3 grid with no obstacles
    int rows = INITIAL_ROWS;
    int cols = INITIAL_COLS;
    
    Loc start = {0, 0};
    Loc end   = {INITIAL_COLS - 1, INITIAL_ROWS - 1};
    
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
    
    // the path describes the locations of the cells from start to end
    Path path = { 0 };
    
    // this string will be displayed to show the path cost
    // "Cost: " => 6
    // "%.2f"   => 13
    // '\0'     => 1
    char cost_str[6 + 13 + 1] = "";
    
    // this string will be displayed to show time taken
    // "Time: " => 6
    // "%.4lf"  => 30
    // '\0'     => 1
    char time_str[6 + 30 + 1] = "";
    
    // represents the time taken by the shortest_path algorithm to find the path
    double time_taken = 0;
    
    // if true the pop-up window showing the cost and time taken will appear
    bool pop_up_open = false;
    
    // the location of the last obstacle set, used to not set/unset the same cell when right click is held
    Loc last_obstacle_changed = null_loc;
    
    // set up the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 820, "Path Finder");
    SetWindowMinSize(1400, 820);
    SetTargetFPS(60);
    SetExitKey(0);
    
    // represents the scroll in the grid scroll panel (unused)
    Vector2 scroll = { 0 };
    
    // represents what's inside the scroll panel, i.e its conent
    Rectangle scroll_panel_content = { 0 };
    
    // represents the sub-rectangle inside scroll_panel_content that is in view (unused)
    Rectangle scroll_view = { 0 };
    
    // represents the bounds of the buttons panel
    Rectangle buttons_panel = {
        .x = 0,
        .y = 0,
        .width  = 245,
        .height = GetScreenHeight()
    };
    
    // represents the bounds of the scroll panel
    Rectangle scroll_panel = {
        .x = buttons_panel.width,
        .y = 0,
        .width  = GetScreenWidth() - buttons_panel.width,
        .height = GetScreenHeight(),
    };
    
    // represents the bounds of the Start button
    Rectangle s_button = {
        .x = (buttons_panel.width  / 2) - (button_size / 2.0f),
        .y = (buttons_panel.height / 2) - (button_size / 2.0f) - button_size,
        .width  = button_size,
        .height = button_size
    };
    
    // represents the bounds of the End button
    Rectangle e_button = {
        .x = s_button.x,
        .y = s_button.y + button_size + button_pad,
        .width  = button_size,
        .height = button_size
    };
    
    // represents the bounds of the Clearbutton
    Rectangle x_button = {
        .x = s_button.x,
        .y = s_button.y - button_pad - button_size,
        .width  = button_size,
        .height = button_size
    };
    
    // represents the bounds of the Path button
    Rectangle p_button = {
        .x = s_button.x,
        .y = x_button.y - button_pad - button_size,
        .width  = button_size,
        .height = button_size
    };
    
    // represents the bounds of the rows spinner
    Rectangle r_spinner = {
        .x = s_button.x,
        .y = e_button.y + button_size + button_pad,
        .width  = 120,
        .height = 45
    };
    
    // represents the bounds of the cols spinner
    Rectangle c_spinner = {
        .x = s_button.x,
        .y = r_spinner.y + r_spinner.height + button_pad,
        .width  = r_spinner.width,
        .height = r_spinner.height
    };
    
    // loading the style
    GuiLoadStyle("../../style_bluish.rgs");
    
    // loading the icons
    GuiLoadIcons("../../iconset.rgi", false);
    GuiSetIconScale(3);
    
    Font font = GuiGetFont();
    int font_size_small = 7;
    int font_size_big = 5;
    
    while(!WindowShouldClose())
    {
        // setting the height for the buttons panel, it scales with window height
        buttons_panel.height = GetScreenHeight();
        
        // setting the bounds for the scroll panel, scales with window size
        scroll_panel.width = GetScreenWidth() - buttons_panel.width;
        scroll_panel.height = GetScreenHeight();
        
        // setting the scroll panel bound, its size depends on rows/cols and cell_size
        scroll_panel_content.width  = cols * (cell_size + line_thickness) + line_thickness;
        scroll_panel_content.height = rows * (cell_size + line_thickness);
        
        // setting the Start button y dimension, scales with window height
        s_button.y = (buttons_panel.height / 2) - (button_size / 2.0f) - button_size;
        
        // setting the End button y dimension, scales with window height
        e_button.y = s_button.y + button_size + button_pad;
        
        // setting the Clear button y dimension, scales with window height
        x_button.y = s_button.y - button_pad - button_size;
        
        // setting the Path button y dimension, scales with window height
        p_button.y = x_button.y - button_pad - button_size;
        
        // setting the row spinner y dimension, scales with window height
        r_spinner.y = e_button.y + button_size + button_pad;
        
        // setting the column spinner y dimension, scales with window height
        c_spinner.y = r_spinner.y + r_spinner.height + button_pad;
        
        // change cell size if '-' or '=' are pressed
        if(cell_size >= 20 && IsKeyDown(KEY_MINUS))
        {
            cell_size-=2;
        }
        if(cell_size <= 256 && IsKeyDown(KEY_EQUAL))
        {
            cell_size+=2;
        }
        
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
        Cell_Click clicked_cell = draw_grid(grid_topleft, cols, rows, scroll_view, scroll_panel.x);
        
        // checks whether a cell is clicked
        bool cell_is_clicked = !locs_eq(clicked_cell.loc, null_loc);
        
        // checks whether the same cell being held, useful for obstacle placing
        bool holding_same_obstacle_cell = cell_is_clicked && clicked_cell.held && locs_eq(clicked_cell.loc, last_obstacle_changed);
        
        scroll_by_dragging_mouse(clicked_cell, &scroll);
        
        draw_obstacles(obstacles, cols, rows, grid_topleft, scroll_view);
        
        draw_path_and_set_cells(path, cost_str, grid_topleft);
        
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
        
        // if the pop-up window is open, draw it and the display the cost and time
        if(pop_up_open)
        {
            font.baseSize = font_size_small;
            GuiSetFont(font);
            Rectangle popup_bounds = {
                .x = (GetScreenWidth() / 2.0f) - (512 / 2.0f),
                .y = (GetScreenHeight() / 2.0f) - (256 / 2.0f),
                .width  = 512,
                .height = 256
            };
            
            pop_up_open = !GuiWindowBox(popup_bounds, "Result");
            
            // setting the font for the cost and time labels
            font.baseSize = font_size_big;
            GuiSetFont(font);
            
            int cost_label_width = GetTextWidth(cost_str);
            Rectangle cost_label_bounds = {
                .x = popup_bounds.x + (popup_bounds.width / 2) - (cost_label_width / 2.0f),
                .y = popup_bounds.y + (popup_bounds.height / 2) - (button_size / 2.0f),
                .width  = cost_label_width,
                .height = 24
            };
            
            GuiLabel(cost_label_bounds, cost_str);
            
            
            sprintf(time_str, "Time: %.4lfs", time_taken);
            int time_label_width = GetTextWidth(time_str);
            
            Rectangle time_label_bounds = {
                .x = popup_bounds.x + (popup_bounds.width / 2) - (time_label_width / 2.0f),
                .y = cost_label_bounds.y + cost_label_bounds.height + button_pad,
                .width  = time_label_width,
                .height = 24
            };
            
            GuiLabel(time_label_bounds, time_str);
        }
        
        // get whether any of the buttons was clicked
        bool find_clicked  = GuiButton(p_button, "#73#");
        bool clear_clicked = GuiButton(x_button, "#113#");
        bool start_clicked = GuiButton(s_button, "#220#");
        bool end_clicked   = GuiButton(e_button, "#221#");
        
        if(clear_clicked)
        {
            // if Start/End is selected, unselect
            no_select();
            
            // remove all obstacles
            clear_obstacles(&obstacles, cols, rows);
            
            // clear the path
            clear_path();
        }
        
        if(start_clicked)
        {
            set_select_mode(START);
        }
        
        if(end_clicked)
        {
            set_select_mode(END);
        }
        
        // if path button was clicked and the Start and End are set, execute the shortest_path algorithm
        if(find_clicked && within_grid(start, cols, rows) && within_grid(end, cols, rows))
        {
            no_select();
            
            time_taken = set_path(&path, obstacles, cols, rows, start, end);
            pop_up_open = true;
            if(path.locs == NULL)
            {
                sprintf(cost_str, "No Path");
            }
        }
        
        // setting the font for rows/cols spinners
        font.baseSize = font_size_small;
        GuiSetFont(font);
        
        //int old_rows = rows, old_cols = cols;
        bool changed_rows_cols = draw_spinners_and_update_rows_cols(r_spinner, c_spinner, &rows, &cols);
        
        // resize the obstacles grid if rows/cols was changed
        if(changed_rows_cols)
        {
            resize_obstacles(&obstacles, cols, rows);
            clear_path();
        }
        
        // have the cursor be normal or an S or an E depending on the select mode
        const int cursor_icon_size = 4;
        switch(select_mode)
        {
            case NO_SELECT:                
                // since we're in normal cursor mode, the user should be able to click on the S/E on the grid
                if(cell_is_clicked && clicked_cell.mouse_button == MOUSE_BUTTON_LEFT && GetMouseX() >= scroll_panel.x)
                {
                    if(locs_eq(clicked_cell.loc, start) && !clicked_cell.held)
                    {
                        set_select_mode(START);
                    }
                    if(locs_eq(clicked_cell.loc, end) && !clicked_cell.held)
                    {
                        set_select_mode(END);
                    }
                }
                break;
            case START:
                // turn cursor into an S
                GuiDrawIcon(220, GetMouseX() - (2 * cursor_icon_size), GetMouseY() - (2 * cursor_icon_size), cursor_icon_size, BLACK);
                
                // since we're in S cursor mode, clicking on a passable cell will put the Start point there
                if(cell_is_clicked && clicked_cell.mouse_button == MOUSE_BUTTON_LEFT && !clicked_cell.held && obstacles[clicked_cell.loc.y][clicked_cell.loc.x] && GetMouseX() >= scroll_panel.x)
                {
                    start.x = clicked_cell.loc.x;
                    start.y = clicked_cell.loc.y;
                    
                    // if the user puts the Start at the same spot that E was, remove E
                    if(locs_eq(start, end))
                        end = null_loc;
                    
                    // make cursor normal next frame, and clear path if it was drawn (since the grid was changed, the path might not apply anymore)
                    no_select();
                    clear_path();
                }
                break;
            case END:
                // turn cursor into an E
                GuiDrawIcon(221, GetMouseX() - (2 *cursor_icon_size), GetMouseY() - (2 * cursor_icon_size), cursor_icon_size, BLACK);
                
                // since we're in E cursor mode, clicking on a passable cell will put the End point there
                if(cell_is_clicked && clicked_cell.mouse_button == MOUSE_BUTTON_LEFT && !clicked_cell.held && obstacles[clicked_cell.loc.y][clicked_cell.loc.x] && GetMouseX() >= scroll_panel.x)
                {
                    end.x = clicked_cell.loc.x;
                    end.y = clicked_cell.loc.y;
                    
                    // if the user puts the End at the same spot that S was, remove S
                    if(locs_eq(start, end))
                        start = null_loc;
                    
                    // make cursor normal next frame, and clear path if it was drawn (since the grid was changed, the path might not apply anymore)
                    no_select();
                    clear_path();
                }
                break;
        }
        
        // right click/hold will set/unset obstacles, as long as its not on Start/End
        if(cell_is_clicked && !holding_same_obstacle_cell && clicked_cell.mouse_button == MOUSE_BUTTON_RIGHT && !locs_eq(clicked_cell.loc, start) && !locs_eq(clicked_cell.loc, end))
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
    
    free(path.locs);
    CloseWindow();
}

Cell_Click draw_grid(Vector2 topleft, int cols, int rows, Rectangle view, int panel_borderx)
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
            if(mousex >= panel_borderx)
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
    for(int i = (view.y - topleft.y) / (cell_size + line_thickness) ; i < rows + 1 && i < (view.y + view.height - topleft.y) / (cell_size + line_thickness) + line_thickness; i++)
    {
        Rectangle line = {
            .x = topleft.x,
            .y = topleft.y + (i * (cell_size + line_thickness)),
            .width  = cols * (cell_size + line_thickness) + line_thickness,
            .height = line_thickness
        };
        
        DrawRectangleRec(line, BLACK);
    }
    
    // drawing the cols
    for(int i = (view.x - topleft.x) / (cell_size + line_thickness) ; i < cols + 1 && i < (view.x + view.width - topleft.x) / (cell_size + line_thickness) + line_thickness; i++)
    {
        Rectangle line = {
            .x = topleft.x + (i * (cell_size + line_thickness)),
            .y = topleft.y,
            .width  = line_thickness,
            .height = rows * (cell_size + line_thickness)
        };
        
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

// scrolls the grid if dragging it with mouse
void scroll_by_dragging_mouse(Cell_Click cell_click, Vector2 *scroll)
{
    static int mousex_old = 0;
    static int mousey_old = 0;
    // if a cell is held with left mouse button, scroll the grid by dragging it
    int mousex = GetMouseX(), mousey = GetMouseY();
    if(!locs_eq(cell_click.loc, null_loc) && cell_click.held && cell_click.mouse_button == MOUSE_BUTTON_LEFT)
    {
        scroll->x += (mousex - mousex_old);
        scroll->y += (mousey - mousey_old);
    }
    mousex_old = mousex;
    mousey_old = mousey;
}

// returns the difference between two times as a double
double diff_timespec(struct timespec time1, struct timespec time0) {
  return (time1.tv_sec - time0.tv_sec)
      + (time1.tv_nsec - time0.tv_nsec) / 1000000000.0;
}

// calls the shortest path algorithm and sets the path
double set_path(Path *path, bool **obstacles, int cols, int rows, Loc start, Loc end)
{
    bool *obstacles1d = obstacles_2d_to_1d(obstacles);
    free(path->locs);
    
    struct timespec before = { 0 }, after = { 0 };
    clock_gettime(CLOCK_REALTIME, &before);
    *path = shortest_path(obstacles1d, cols, rows, start, end);
    clock_gettime(CLOCK_REALTIME, &after);
    
    free(obstacles1d);
    
    return diff_timespec(after, before);
}

// draws the path as green squares on the grid, storing the path cells in the 'path_cells'
void draw_path_and_set_cells(Path path, char *cost_str, Vector2 topleft)
{
    // if a path exists, draw it on the grid
    if(path.locs != NULL)
    {
        for(int i = 0 ; i < path.nb ; i++)
        {
            // current cell to draw on the grid
            Loc current = path.locs[i];
            
            // this rectangle represents the coordinates and size of the cell
            Rectangle path_cell_rect = {
                .x = topleft.x + line_thickness + (current.x * (cell_size + line_thickness)),
                .y = topleft.y + line_thickness + (current.y * (cell_size + line_thickness)),
                .width  = cell_size,
                .height = cell_size
            };
            
            // draw the path cell as GREEN
            DrawRectangleRec(path_cell_rect, GREEN);
        }
        
        // set the path cost to the cost string
        sprintf(cost_str, "Cost: %.2f", path.cost);
    }
}

bool draw_spinners_and_update_rows_cols(Rectangle r_spinner, Rectangle c_spinner, int *rows, int *cols)
{
    static bool edit_rows = false;
    static bool edit_cols = false;
    static int edited_rows = INITIAL_ROWS;
    static int edited_cols = INITIAL_COLS;
    
    int old_rows = *rows, old_cols = *cols;
    
    // drawing the spinners and updating the rows/cols
    // if user clicks on the spinner, he will be able to edit the value directly
    edit_rows = GuiSpinner(r_spinner, "Rows ", edit_rows ? &edited_rows : rows, 1, INT_MAX, edit_rows) ? true : edit_rows;
    edit_cols = GuiSpinner(c_spinner, "Cols ", edit_cols ? &edited_cols : cols, 1, INT_MAX, edit_cols) ? true : edit_cols;
    
    // pressing enter will confirm edit
    bool enter_pressed = IsKeyPressed(KEY_ENTER);
    if(edit_rows && enter_pressed)
    {
        edit_rows = false;
        if(edited_rows < 1)
            edited_rows = 1;
        *rows = edited_rows;
    }
    if(edit_cols && enter_pressed)
    {
        edit_cols = false;
        if(edited_cols < 1)
            edited_cols = 1;
        *cols = edited_cols;
    }
    
    // if mouse is hovering over spinner and scrolls up/down, the value should change
    int mousex = GetMouseX();
    int mousey = GetMouseY();
    float spinner_scroll = GetMouseWheelMoveV().y;
    
    if(mousex >= r_spinner.x && mousex <= r_spinner.x + r_spinner.width)
        if(mousey >= r_spinner.y && mousey <= r_spinner.y + r_spinner.height)
            *rows += iclampf(spinner_scroll, -1, 1);
    
    if(mousex >= c_spinner.x && mousex <= c_spinner.x + c_spinner.width)
        if(mousey >= c_spinner.y && mousey <= c_spinner.y + c_spinner.height)
            *cols += iclampf(spinner_scroll, -1, 1);
    
    bool changed_dims = *rows != old_rows || *cols != old_cols;
    if(changed_dims)
    {
        if(!edit_rows)
            edited_rows = *rows;
        if(!edit_cols)
            edited_cols = *cols;
    }
    
    return changed_dims;
}

// draws the obstacles on the grid as red squares
void draw_obstacles(bool **obstacles, int cols, int rows, Vector2 topleft, Rectangle view)
{
    // draw the obstacles that are in view
    for(int i = (view.y - topleft.y) / (cell_size + line_thickness) ; i < rows && i < (view.y + view.height - topleft.y) / (cell_size + line_thickness) + line_thickness ; i++)
    {
        for(int j = (view.x - topleft.x) / (cell_size + line_thickness) ; j < cols && j < (view.x + view.width - topleft.x) / (cell_size + line_thickness) + line_thickness ; j++)
        {
            if(!obstacles[i][j])
            {
                Rectangle obstacle_rect = {
                    .x = topleft.x + line_thickness + (j * (cell_size + line_thickness)),
                    .y = topleft.y + line_thickness + (i * (cell_size + line_thickness)),
                    .width  = cell_size,
                    .height = cell_size
                };
                
                DrawRectangleRec(obstacle_rect, RED);
            }
        }
    }
}

// removes all the obstacles from the grid
void clear_obstacles(bool ***obstacles, int cols, int rows)
{
    for(int i = 0 ; i < rows ; i++)
    {
        for(int j = 0 ; j < cols ; j++)
        {
            (*obstacles)[i][j] = true;
        }
    }
}

// clamps a float between 2 int values
int iclampf(float f, int min, int max)
{
    return f > max ? max : (f < min ? min : f);
}
