# Path-Finder GUI
A graphical program for visualizing the path finding algorithm

**Requires OpenGL**
## Building
To build the project you need either:
* Any C compiler on Linux (tested with GCC)
* MinGW on Windows

Steps to build on Linux:
1. run `./premake5 gmake2`
2. run `make`
3. executable will be generated in `./_bin/Debug/` named `_app`

Steps to build on Windows:
1. run `premake-mingw.bat`
2. run `make`
3. executable will be generated in `./_bin/Debug/` named `_app`

## Usage
* Clicking the button with the arrow on it will draw the path and display the cost.
* Right clicking on the grid will place an obstacle there
* Clicking the X button will clear the path and any obstacles on the grid.
* Clicking the S button or the S on the grid will let you relocate the start point
* Clicking the E button or the E on the grid will let you relocate the end point
* Scrolling on the Rows spinner or clicking the arrow buttons will increase/decrease the rows
* Scrolling on the Cols spinner or clicking the arrow buttons will increase/decrease the cols
* Pressing - will zoom out
* Pressing = will zoom in
