#include "widgets.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>

#define GRID_WIDTH  15
#define GRID_HEIGHT 15
#define CALC_X_POS(iterator) iterator%GRID_WIDTH
#define CALC_Y_POS(iterator) iterator/GRID_WIDTH
#define DRAW_TILE(pos_x, pos_y, color) hagl_fill_rectangle_xywh(display_handle, pos_x, pos_y, tile_size, tile_size, color);


typedef enum : uint8_t 
{
    DIRECTION_PADDING_LOWER,
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_PADDING_UPPER,
}direction_t;

typedef struct
{
    int x;
    int y;
}coordinates;

void calculate_head_pos(std::vector<coordinates>* segments, direction_t direction)
{
    switch (direction)
    {
    case DIRECTION_UP:
        segments->back().y--;
        break;
    
    case DIRECTION_RIGHT:
        segments->back().x++;
        break;

    case DIRECTION_DOWN:
        segments->back().y++;
        break;
    
    case DIRECTION_LEFT:
        segments->back().x--;
        break;

    default:
        break;
    }
}

direction_t calculate_move_direction(direction_t current_direction, input_event_t input)
{
    switch (input)
        {
        case SCROLL_DOWN_EVENT:
            current_direction = static_cast<direction_t>(current_direction - 1);
            if (current_direction == DIRECTION_PADDING_LOWER)
            {
                current_direction = static_cast<direction_t>(DIRECTION_PADDING_UPPER - 1);
            }                
            break;
        
        case SCROLL_UP_EVENT:
            current_direction = static_cast<direction_t>(current_direction + 1);
            if (current_direction == DIRECTION_PADDING_UPPER)
            {
                current_direction = static_cast<direction_t>(DIRECTION_PADDING_LOWER + 1);
            }  
            break;

        default:
            break;
        }
    return current_direction;
}

void find_new_food_position(std::vector<coordinates>* segments, coordinates* food)
{
    food->x = (esp_random()/0x1000000)%GRID_WIDTH;
    food->y = (esp_random()/0x1000000)%GRID_HEIGHT;

    for (int i = 0; i < segments->size(); i++)
    {
        if (food->x != (*segments)[i].x && food->y != (*segments)[i].y)
        {
            return;
        }

        else
        {
            find_new_food_position(segments, food);
        }
        
    }
}

void erase_snake_tail(std::vector<coordinates>* segments, hagl_color_t color, int tile_size, hagl_backend_t* display_handle)
{
    coordinates current_pos{.x=(*segments).front().x, .y=(*segments).front().y};
    static coordinates prev_pos{.x=(*segments).front().x+1, .y=(*segments).front().y+1};

    if (prev_pos.x != current_pos.x || prev_pos.y != current_pos.y)
    {
        DRAW_TILE((*segments).front().x*tile_size, (*segments).front().y*tile_size, color); // Erase prev position
        prev_pos.x = current_pos.x;
        prev_pos.y = current_pos.y;
    }
}

void sort_segments(std::vector<coordinates>* segments, coordinates prev_head_cords)
{
    for (int i = (*segments).size()-2; i >= 0; i--)
    {
        int temp_pos_x = (*segments)[i].x;
        int temp_pos_y = (*segments)[i].y;
        
        (*segments)[i].x = prev_head_cords.x;
        (*segments)[i].y = prev_head_cords.y;

        prev_head_cords.x = temp_pos_x;
        prev_head_cords.y = temp_pos_y;
    }
}

snake_game_widget::snake_game_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex) :
                                    widget(widget_display, display_mutex)
{
    widget::requires_inputs = true;
    xTaskCreate(call_run_widget, "test_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

snake_game_widget::~snake_game_widget()
{
    vTaskDelete(task_handle);
}

void snake_game_widget::run_widget()
{
    int tile_size = (DISPLAY_WIDTH/GRID_WIDTH)*GRID_HEIGHT<DISPLAY_HEIGHT ? DISPLAY_WIDTH/GRID_WIDTH : DISPLAY_HEIGHT/GRID_HEIGHT;

    direction_t move_direction = DIRECTION_RIGHT;

    hagl_color_t snake_color = hagl_color(display_handle, 41, 233, 99);
    hagl_color_t food_color = hagl_color(display_handle, 0, 0, 255);
    hagl_color_t background_color = hagl_color(display_handle, 0, 0, 0);

    coordinates head {.x = GRID_WIDTH/2, .y = GRID_HEIGHT/2};

    std::vector<coordinates> snake_segments{head};
    head.x += 1;
    snake_segments.push_back(head);
    head.x += 1;
    snake_segments.push_back(head);

    bool is_game_over = false;

    coordinates food;
    find_new_food_position(&snake_segments, &food);    

    for (int i = 0; i < snake_segments.size(); i++)
    {
        DRAW_TILE(snake_segments[i].x*tile_size, snake_segments[i].y*tile_size, snake_color);
    }

    while (1)
    {
        while (!is_game_over)
        {
            move_direction = calculate_move_direction(move_direction, input_event);
            input_event = INIT_EVENT;

            if (snake_segments.back().x == food.x && snake_segments.back().y == food.y)
            {
                coordinates new_segment{.x=snake_segments.back().x,.y=snake_segments.back().y};
                snake_segments.push_back(new_segment);
                find_new_food_position(&snake_segments, &food);
            }
            
            head.x = snake_segments.back().x;
            head.y = snake_segments.back().y;

            calculate_head_pos(&snake_segments, move_direction);

            if (snake_segments[0].x == GRID_WIDTH || snake_segments[0].x < 0 || snake_segments[0].y == GRID_HEIGHT || snake_segments[0].y < 0)
            {
                is_game_over = true;
            }

            else
            {
                DRAW_TILE(food.x*tile_size, food.y*tile_size, food_color); // Draw food

                erase_snake_tail(&snake_segments, background_color, tile_size, display_handle);

                sort_segments(&snake_segments, head);

                DRAW_TILE(snake_segments.back().x*tile_size, snake_segments.back().y*tile_size, snake_color)
            }
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    vTaskDelay(pdMS_TO_TICKS(250));
    } 
}