#include "widgets.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// snake_game_widget::snake_game_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex, hagl_window_t display_bounds) :
//                                                widget(widget_display, display_mutex), clip(display_bounds)
// {
//     tile_size_x = 5; // Placeholder tile sizes
//     tile_size_y = 5;

//     xTaskCreate(call_run_widget, "test_widget", 8192, this, 3, &task_handle); // Create task to call run_widget
// }

// snake_game_widget::~snake_game_widget()
// {
//     if (xSemaphoreTake(display_mutex, portMAX_DELAY))
//     {
//         vTaskDelete(task_handle);
//         hagl_set_clip(display_handle, clip.x0, clip.y0, clip.x1, clip.y1); // Set drawable area
//         hagl_fill_rectangle_xyxy(display_handle, clip.x0, clip.y0, clip.x1, clip.y1, hagl_color(display_handle,0,0,0)); // Clear screen
//         xSemaphoreGive(display_mutex);
//     } // Clear screen
// }

// void snake_game_widget::draw_tile_map()
// {
//     if (xSemaphoreTake(display_mutex, portMAX_DELAY))
//     {
//         hagl_set_clip(display_handle, clip.x0, clip.y0, clip.x1, clip.y1); // Set drawable area
        
//         tile* current_tile = game_grid->get_tile(0, 0);

//         for(int i = 0; i < 240*240; i++)
//         {
//             hagl_fill_rectangle_xywh(display_handle,
//                                     (current_tile->x*tile_size_x)+clip.x0,
//                                     (current_tile->y*tile_size_y)+clip.y0,
//                                     tile_size_x, tile_size_y,
//                                     *((uint8_t*)current_tile->metadata)*255);

//             current_tile = game_grid->get_next_tile(current_tile);
            
//             if (current_tile == NULL)
//             {
//                 break;
//             }
//         }
    
//     xSemaphoreGive(display_mutex);
//     }
// }

// void snake_game_widget::run_widget()
// {
//     game_grid = new grid(24, 24, 1);
//     draw_tile_map();

//     while (1)
//     {   
//         if (xSemaphoreTake(display_mutex, portMAX_DELAY))
//         {
            

//         xSemaphoreGive(display_mutex);
//         }

//         #ifdef CHECK_TASK_SIZES
//         ESP_LOGI("test_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
//         #endif
        
//         vTaskDelay(portMAX_DELAY);
//     }
// }

snake_game_widget::snake_game_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex, hagl_window_t display_bounds) :
                                    widget(widget_display, display_mutex), clip(display_bounds)
{
    game_grid = new virtual_grid(100, 100);
    xTaskCreate(call_run_widget, "test_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

snake_game_widget::~snake_game_widget()
{
    vTaskDelete(task_handle);
    delete game_grid;
}

void snake_game_widget::run_widget()
{
    while (1)
    {
        tile_id_t id = game_grid->add_tile(99, 99, true);
        // ESP_LOGI("", "%i", id);

        ESP_LOGI("", "%i", (game_grid->find_tile_by_id(id) == (virtual_tile*)GRID_ERR_TILE_NOT_FOUND)? 0 : (int)game_grid->find_tile_by_id(id)->coord_x);
        vTaskDelay(pdMS_TO_TICKS(100));
    } 
}