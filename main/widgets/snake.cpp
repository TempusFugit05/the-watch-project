#include "widgets.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>

#define GRID_WIDTH  15
#define GRID_HEIGHT 15
#define DRAW_TILE(pos_x, pos_y, color) hagl_fill_rectangle_xywh(display_handle, pos_x, pos_y, tile_size, tile_size, color);
#define DRAW_TILE_WITH_OUTLINE(pos_x, pos_y, color, outline_color) hagl_fill_rectangle_xywh(display_handle, pos_x, pos_y, tile_size, tile_size, color); hagl_draw_rectangle_xywh(display_handle, pos_x, pos_y, tile_size, tile_size, outline_color);

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

void draw_eyes(hagl_backend_t* display_handle, std::vector<coordinates>* segments, int tile_size, hagl_color_t eye_color, direction_t direction)
{
    // AWFUL!!!!!!!!!!!
    coordinates head_pos{.x=(*segments).back().x, .y=(*segments).back().y};
    
    if (direction == DIRECTION_UP || direction == DIRECTION_DOWN)
    {
        coordinates eye_pos{.x=head_pos.x*tile_size+1, .y=head_pos.y*tile_size+tile_size/3};
        coordinates eye_pos_2{.x=(head_pos.x+1)*tile_size-tile_size/3-1, .y=head_pos.y*tile_size+tile_size/3};

        hagl_fill_rectangle_xywh(display_handle, eye_pos.x, eye_pos.y, tile_size/3, tile_size/3, eye_color);
        hagl_fill_rectangle_xywh(display_handle, eye_pos_2.x, eye_pos_2.y, tile_size/3, tile_size/3, eye_color);
    }

    else
    {
        coordinates eye_pos{.x=head_pos.x*tile_size+tile_size/3, .y=head_pos.y*tile_size+1};
        coordinates eye_pos_2{.x=head_pos.x*tile_size+tile_size/3, .y=(head_pos.y+1)*tile_size-tile_size/3-1};

        hagl_fill_rectangle_xywh(display_handle, eye_pos.x, eye_pos.y, tile_size/3, tile_size/3, eye_color);
        hagl_fill_rectangle_xywh(display_handle, eye_pos_2.x, eye_pos_2.y, tile_size/3, tile_size/3, eye_color);
    }
    
}

void fill_tail_line(hagl_backend_t* display_handle, std::vector<coordinates>* segments, int tile_size, hagl_color_t outline_color)
{
    // AAAAAAAAAAAAAAAAAAAA
    coordinates current_seg = {.x=(*segments).front().x, .y=(*segments).front().y};
    coordinates next_seg = {.x=(*segments)[1].x, .y=(*segments)[1].y};

    if (current_seg.x - next_seg.x == 1)
    {
        hagl_draw_vline_xyh(display_handle, (current_seg.x+1)*tile_size-1, current_seg.y*tile_size+1, tile_size-2, outline_color);
        hagl_draw_vline_xyh(display_handle, (current_seg.x+1)*tile_size-1, current_seg.y*tile_size+1, tile_size-2, outline_color);    
    }
    else if (current_seg.x - next_seg.x == -1)
    {
        hagl_draw_vline_xyh(display_handle, current_seg.x*tile_size, current_seg.y*tile_size+1, tile_size-2, outline_color);
        hagl_draw_vline_xyh(display_handle, current_seg.x*tile_size+1, current_seg.y*tile_size+1, tile_size-2, outline_color);
    }
    else if (current_seg.y - next_seg.y == 1)
    {
        hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, (current_seg.y+1)*tile_size-1, tile_size-2, outline_color);
        hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, (current_seg.y+1)*tile_size-1, tile_size-2, outline_color);
    }
    else
    {
        hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, current_seg.y*tile_size, tile_size-2, outline_color);
        hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, current_seg.y*tile_size+1, tile_size-2, outline_color);
    }
}

void clean_segment_lines(hagl_backend_t* display_handle, std::vector<coordinates>* segments, int tile_size, hagl_color_t snake_color, hagl_color_t outline_color)
{
    // ABSOLUTELY TERRIBLE!!!
    for (int i = (*segments).size()-1; i > (*segments).size()-3; i--)
    {
        coordinates current_seg={.x=(*segments)[i].x, .y=(*segments)[i].y};
        coordinates prev_seg={.x=(*segments)[i-1].x, .y=(*segments)[i-1].y};
        if (current_seg.x - prev_seg.x == 1)
        {
            hagl_draw_vline_xyh(display_handle, current_seg.x*tile_size, current_seg.y*tile_size+1, tile_size-2, snake_color);
            hagl_draw_vline_xyh(display_handle, current_seg.x*tile_size-1, current_seg.y*tile_size+1, tile_size-2, snake_color);    
        }
        else if (current_seg.x - prev_seg.x == -1)
        {
            hagl_draw_vline_xyh(display_handle, (current_seg.x+1)*tile_size, current_seg.y*tile_size+1, tile_size-2, snake_color);
            hagl_draw_vline_xyh(display_handle, (current_seg.x+1)*tile_size-1, current_seg.y*tile_size+1, tile_size-2, snake_color);
        }
        else if (current_seg.y - prev_seg.y == 1)
        {
            hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, current_seg.y*tile_size, tile_size-2, snake_color);
            hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, current_seg.y*tile_size-1, tile_size-2, snake_color);
        }
        else
        {
            hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, (current_seg.y+1)*tile_size, tile_size-2, snake_color);
            hagl_draw_hline_xyw(display_handle, current_seg.x*tile_size+1, (current_seg.y+1)*tile_size-1, tile_size-2, snake_color);
        }
    }
    fill_tail_line(display_handle, segments, tile_size, outline_color);
}

void find_new_food_position(std::vector<coordinates>* segments, coordinates* food)
{
    food->x = esp_random()%GRID_WIDTH;
    food->y = esp_random()%GRID_HEIGHT;

    for (int i = 0; i < segments->size(); i++)
    {
        if (food->x == (*segments)[i].x && food->y == (*segments)[i].y)
        {
            find_new_food_position(segments, food);
        } // If the food has the same xy values as any segment, find new position.
    }
}

void sort_segments(std::vector<coordinates>* segments, coordinates prev_head_cords)
{
    // Rearrange the segments to take the position of the one after them
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

bool check_self_collisions(std::vector<coordinates>* segments)
{
    coordinates head_pos{.x=(*segments).back().x, .y=(*segments).back().y};

    for (int i = 0; i < (*segments).size()-2; i++)
    {
        coordinates segment_pos{.x=(*segments)[i].x, .y=(*segments)[i].y};
        if (head_pos.x == segment_pos.x && head_pos.y == segment_pos.y)
        {
            return true; // Return true if there was a collision
        }
    }

    return false; // Return false if no collision detected
}

snake_game_widget::snake_game_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex) :
                                    widget(widget_display, display_mutex)
{
    widget::requires_inputs = true;
    xTaskCreate(call_run_widget, "test_widget", 4096, this, 3, &task_handle); // Create task to call run_widget
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
    hagl_color_t outline_color = hagl_color(display_handle, 255, 55, 169);
    hagl_color_t food_color = hagl_color(display_handle, 0, 0, 255);
    hagl_color_t background_color = hagl_color(display_handle, 0, 0, 0);

    coordinates head {.x = GRID_WIDTH/2-3, .y = GRID_HEIGHT/2-3};

    std::vector<coordinates> snake_segments{head};
    head.x += 1;
    snake_segments.push_back(head);
    head.x += 1;
    snake_segments.push_back(head);

    bool is_game_over = false;
    volatile bool new_segment_created = false;

    coordinates food;
    find_new_food_position(&snake_segments, &food);    

    for (int i = 1; i < snake_segments.size(); i++)
    {
        DRAW_TILE_WITH_OUTLINE(snake_segments[i].x*tile_size, snake_segments[i].y*tile_size, snake_color, outline_color);
    }
    clean_segment_lines(display_handle, &snake_segments, tile_size, snake_color, outline_color);
    DRAW_TILE(food.x*tile_size, food.y*tile_size, food_color); // Draw food

    while (1)
    {
        while (!is_game_over)
        {            
            move_direction = calculate_move_direction(move_direction, input_event);
            input_event = INIT_EVENT; // Reset input

            head.x = snake_segments.back().x;
            head.y = snake_segments.back().y;

            if (snake_segments.back().x == food.x && snake_segments.back().y == food.y)
            {
                coordinates new_segment{.x=snake_segments.back().x,.y=snake_segments.back().y};
                snake_segments.push_back(new_segment);
                find_new_food_position(&snake_segments, &food);
                DRAW_TILE(food.x*tile_size, food.y*tile_size, food_color); // Draw food
                new_segment_created = true;
            }
        
            calculate_head_pos(&snake_segments, move_direction);

            if (snake_segments.back().x == GRID_WIDTH || snake_segments.back().x < 0 ||
             snake_segments.back().y == GRID_HEIGHT || snake_segments.back().y < 0 || check_self_collisions(&snake_segments))
            {
                is_game_over = true;
                break;
            }

            if (!new_segment_created)
            {
                DRAW_TILE(snake_segments.front().x*tile_size, snake_segments.front().y*tile_size, background_color); // Erase prev position
                sort_segments(&snake_segments, head);
            }


            DRAW_TILE_WITH_OUTLINE(snake_segments.back().x*tile_size, snake_segments.back().y*tile_size, snake_color, outline_color) // Draw head
            draw_eyes(display_handle, &snake_segments, tile_size, background_color, move_direction);
            DRAW_TILE_WITH_OUTLINE(snake_segments[snake_segments.size()-2].x*tile_size, snake_segments[snake_segments.size()-2].y*tile_size, snake_color, outline_color) // Clear previous segment

            clean_segment_lines(display_handle, &snake_segments, tile_size, snake_color, outline_color);
            new_segment_created = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    vTaskDelay(pdMS_TO_TICKS(250));
    } 
}