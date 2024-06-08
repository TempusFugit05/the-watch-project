#ifndef GRID_H
#define GRID_H

#include <cstdio>
#include <list>

typedef struct
{
    uint8_t x;
    uint8_t y;
}pure_tile; // x and y values are only suitable for small displays!

class pure_grid
{
private:
    int size_x;
    int size_y;
    pure_tile** grid_map;
    
public:
    pure_grid(int size_x, int size_y);
    ~pure_grid();
    int get_size_x();
    int get_size_y();
    pure_tile* get_tile(int index_x, int index_y); // Get a specific pure_tile from the pure_grid
    pure_tile* get_next_tile(const pure_tile* reference_tile);
}; // Might be useless, not sure yet...
// This class might be useless since you can just calculate the x,y values based of the index of the tile
// and the x,y size of the grid...


typedef struct
{
    uint8_t x;
    uint8_t y;
    // list<int> mylist{1,2,3};
    void* metadata;
}tile; // x and y values are only suitable for small displays!

class grid
{
private:
    int size_x;
    int size_y;
    tile** grid_map;
    
public:
    grid(int size_x, int size_y, uint8_t metadata_size_bytes);
    ~grid();
    int get_size_x();
    int get_size_y();
    tile* get_tile(int index_x, int index_y); // Get a specific pure_tile from the pure_grid
    tile* get_next_tile(const tile* reference_tile);
};
// This might also be useless based off the usefulness of the pure grid!
// It would probably be simpler to just create an array of some data type and calculate the xy values based on the index...

/*TODO: Create a virtual grid system. As well as a linked list*/

// class virtual_grid
// {
// private:
//     /* data */
// public:
//     virtual_grid();
//     ~virtual_grid();
// };

#endif // GRID_H