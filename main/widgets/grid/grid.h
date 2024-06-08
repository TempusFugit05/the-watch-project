#ifndef GRID_H
#define GRID_H

#include <cstdio>
#include <list>


typedef uint16_t tile_id_t;

typedef struct
{
    uint16_t coord_x;
    uint16_t coord_y;
    uint16_t id;
}virtual_tile;

typedef enum : uint16_t
{
    GRID_OK,
    GRID_ERR_OUT_OF_BOUNDS,
    GRID_ERR_OVERLAP,
    GRID_ERR_TILE_NOT_FOUND,
    NUM_ERRS,
}grid_error_code_t;

typedef enum : uint8_t
{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
}grid_direction_t;

class virtual_grid
{
private:
    std::list<tile_id_t> free_tiles;
    std::list<virtual_tile> active_tiles;

    uint16_t grid_size_x;
    uint16_t grid_size_y;
    tile_id_t current_max_id = NUM_ERRS; // The minimum id number would the number of errors defined.
                                         // This is done to be able to return tile ids or errors if necessary.

    tile_id_t find_tile_by_xy(uint16_t pos_x, uint16_t pos_y);
    tile_id_t get_new_id();

public:
    virtual_grid(uint16_t size_x, uint16_t size_y);
    ~virtual_grid();

    tile_id_t add_tile(uint16_t pos_x, uint16_t pos_y, bool force_add);
    grid_error_code_t delete_tile(tile_id_t);
    grid_error_code_t move_tile_xy(tile_id_t tile_id, uint16_t new_pos_x, uint16_t new_pos_y);
    grid_error_code_t move_tile_direction(tile_id_t tile_id, grid_direction_t direction, uint16_t amount);
    virtual_tile* find_tile_by_id(tile_id_t tile_id);

    // /**
    //  * @brief Check if two tiles overlap.
    //  * @param tile_id id of the tile being checked.
    //  * @param ignore_list id's of tiles to be ignored when checking overlap
    //  * @return GRID_OK if there is no overlap or the tile id the tile overlaps with.
    // */

    // tile_id_t check_tile_overlap(tile_id_t tile_id, ...);
};

#endif // GRID_H