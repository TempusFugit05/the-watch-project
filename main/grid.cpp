#include "grid.h"
#include "freertos/FreeRTOS.h"
#include "random"
#include "esp_log.h"

pure_grid::pure_grid(int size_x, int size_y) : size_x(size_x), size_y(size_y)
{
    int tile_index_x = 0;
    int tile_index_y = 0;

    grid_map = (pure_tile**)pvPortMalloc(sizeof(pure_tile*)*(size_x));
    for(int i = 0; i < size_x; i++)
    {
        grid_map[i] = (pure_tile*)pvPortMalloc(sizeof(pure_tile) * (size_y));
    }
    
    ESP_LOGI("", "buffer size:: %i", sizeof(pure_tile*) * (size_x) + sizeof(pure_tile) * size_x*size_y); // Calculate the buffer size
        
    for (int i=0; i < size_x*size_y; i++)
    {
        tile_index_y = i%size_y;

        if (i%((size_x*size_y)/size_x) == 0 && i!=0)
        {
            tile_index_x++;
        }

        grid_map[tile_index_x][tile_index_y].x = tile_index_x;
        grid_map[tile_index_x][tile_index_y].y = tile_index_y;
    }

    heap_caps_check_integrity_all("");
}

pure_grid::~pure_grid()
{
    // Free the alocated memory
    for(int i = 0; i < size_x; i++)
    {
        vPortFree(grid_map[i]);
    }
    vPortFree(grid_map);
}

int pure_grid::get_size_x()
{
    return size_x;
}

int pure_grid::get_size_y()
{
    return size_y;
}

pure_tile* pure_grid::get_tile(int index_x, int index_y)
{
    return &grid_map[index_x][index_y];
}

pure_tile* pure_grid::get_next_tile(const pure_tile* reference_tile)
{
    int index_x = reference_tile->x;
    int index_y = reference_tile->y;

    if(index_y > size_y-2)
    {
        index_y = 0;
        index_x++;
    } // Wrap to next tile on x axis

    else
    {
        index_y++;
    }

    if(index_x > size_x-1)
    {
        return NULL;
    } // Return error if trying to access a tile beyond bounds

    else
    {
        return &grid_map[index_x][index_y];
    } // Return next tile
}

grid::grid(int size_x, int size_y, uint8_t metadata_size_bytes) : size_x(size_x), size_y(size_y)
{
    grid_map = (tile**)pvPortMalloc(sizeof(tile*)*(size_x));
    for(int i = 0; i < size_x; i++)
    {
        grid_map[i] = (tile*)pvPortMalloc(sizeof(tile) * (size_y));
    }

    for (int i=0; i < size_x*size_y; i++)
    {
        int tile_index_x = i/size_y;
        int tile_index_y = i%size_y;

        grid_map[tile_index_x][tile_index_y].metadata = pvPortMalloc(metadata_size_bytes); // Alocate memory for metadata
    }

    // Calculate the buffer size
    ESP_LOGI("", "buffer size:: %i", (sizeof(tile*)*(size_x)) + (sizeof(tile)*size_x*size_y) + (metadata_size_bytes*size_x*size_y)); 
    
    for (int i=0; i < size_x*size_y; i++)
    {
        int tile_index_x = i/size_y;
        int tile_index_y = i%size_y;

        grid_map[tile_index_x][tile_index_y].x = tile_index_x;
        grid_map[tile_index_x][tile_index_y].y = tile_index_y;
        *((int8_t*)grid_map[tile_index_x][tile_index_y].metadata) = rand()%255;
    }
    
    heap_caps_check_integrity_all("");
}

grid::~grid()
{
    // Free the alocated memory

    for (int i=0; i < size_x*size_y; i++)
    {
        int tile_index_x = i/size_y;
        int tile_index_y = i%size_y;

        vPortFree(grid_map[tile_index_x][tile_index_y].metadata); // Free memory for metadata
    }

    for(int i = 0; i < size_x; i++)
    {
        vPortFree(grid_map[i]);
    }
    vPortFree(grid_map);
}

tile* grid::get_tile(int index_x, int index_y)
{
    return &grid_map[index_x][index_y];
}

tile* grid::get_next_tile(const tile* reference_tile)
{
    int index_x = reference_tile->x;
    int index_y = reference_tile->y;

    if(index_y > size_y-2)
    {
        index_y = 0;
        index_x++;
    } // Wrap to next tile on x axis

    else
    {
        index_y++;
    }

    if(index_x > size_x-1)
    {
        return NULL;
    } // Return error if trying to access a tile beyond bounds

    else
    {
        return &grid_map[index_x][index_y];
    } // Return next tile
}