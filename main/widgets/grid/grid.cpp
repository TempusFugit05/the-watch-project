#include "grid.h"
#include "freertos/FreeRTOS.h"
#include "random"
#include "esp_log.h"

virtual_grid::virtual_grid(uint16_t size_x, uint16_t size_y) : grid_size_x(size_x), grid_size_y(size_y){}

virtual_grid::~virtual_grid()
{
    active_tiles.clear();
    free_tiles.clear();
}

tile_id_t virtual_grid::get_new_id()
{
    if (free_tiles.empty())
    {
        current_max_id ++;
        return current_max_id;
    } // If no free id's exist, create a new id and update the current max id

    else
    {
        tile_id_t id = free_tiles.back();
        free_tiles.pop_back();
        return id;
    } // Get an unused id from the free id's list 
}

virtual_tile* virtual_grid::find_tile_by_id(tile_id_t tile_id)
{
    // Iterate through the list of tiles
    for(auto it = active_tiles.begin(); it != active_tiles.end(); it++)
    {
        ESP_LOGI("", "%i, %i", it->id, tile_id);

        if (it->id == tile_id)
        {
            return &(*it); // Return a pointer to the tile that matches the id
        }
    }
    return (virtual_tile*)GRID_ERR_TILE_NOT_FOUND; // Return error if tile not found
}

tile_id_t virtual_grid::find_tile_by_xy(uint16_t pos_x, uint16_t pos_y)
{
    /*Compare the xy values of the tiles*/
    for (auto const& tile : active_tiles)
    {
        if (tile.coord_x == pos_x && tile.coord_y == pos_y)
        {
            return tile.id;
        }
    }
    return (tile_id_t)GRID_OK; // Return OK if there is no overlap
}

tile_id_t virtual_grid::add_tile(uint16_t pos_x, uint16_t pos_y, bool force_add)
{

    if (active_tiles.empty())
    {
        goto create_tile;
    }
    
    /*Check if new tile is reaching over the grid bounds*/
    if (pos_x > grid_size_x-1 || pos_y > grid_size_y-1)
    {
        return GRID_ERR_OUT_OF_BOUNDS;
    }

    /*Check if the new tile overlaps with any existing ones*/
    if(find_tile_by_xy(pos_x, pos_y) != GRID_OK)
    {
        if (force_add)
        {
            goto create_tile;
        }

        else
        {
            return GRID_ERR_OVERLAP;
        }
    }

    create_tile:
    tile_id_t new_tile_id = get_new_id();
    active_tiles.push_back(virtual_tile{.coord_x=pos_x, .coord_y=pos_y, .id=new_tile_id}); // Add new tile
    return new_tile_id;
}
