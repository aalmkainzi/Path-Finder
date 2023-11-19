# Path-Finder CLI
A command line program that takes input from stdin and writes output to stdout.

**Requires a terminal that supports UTF-8**

## Building
simply run:
```
make path
```
and an executable named `path` will be generated in `./bin`.

## Usage
the first thing the program will ask is:
```
Number of rows:
```
To which you should provide an integer and press enter.

And the same goes for the next prompt:
```
Number of cols:
```
After that, the program will ask you to enter each row to the grid.

You do that by writing `1` for passable cells, `0` for unpassable, `S/s` for start and `E/e` for end.

The number of rows that will be prompted is the number you entered, and each row must have `cols` number of characters.

After writing a row, press enter and the next row will be prompted until the grid is complete.

## Example
```
Number of rows: 
5
Number of cols: 
5

Enter the grid:

1 for passable
0 for unpassable
S for start point
E for end point

row #1:
S1111    
row #2:
00001
row #3:
00001
row #4:
00001
row #5:
E1111

cost: 10.83

|S|🡒|🡒|🡖| |
|🞨|🞨|🞨|🞨|🡓|
|🞨|🞨|🞨|🞨|🡓|
|🞨|🞨|🞨|🞨|🡗|
|E|🡐|🡐|🡐| |
```
