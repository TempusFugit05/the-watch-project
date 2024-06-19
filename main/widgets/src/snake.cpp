#include "widgets.h"
#include "fonts.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>
#include <string.h>
#include "math.h"

// #define CHECK_TASK_SIZES

#define GRID_WIDTH  15
#define GRID_HEIGHT 15

#define SCORE_FONT font6x9
#define DIALOGUE_FONT font9x18b

#define BASE_GAME_ITERATIONS_PER_SECOND 5 // Game speed as in iterations per second
#define MAX_GAME_ITERATIONS_PER_SECOND 12
#define SCORE_FOR_MAX_SPEED sqrt(GRID_WIDTH*GRID_HEIGHT)

#define BASE_REFRESH_RATE 1000/BASE_GAME_ITERATIONS_PER_SECOND
#define MAX_REFRESH_RATE 1000/MAX_GAME_ITERATIONS_PER_SECOND


#define BUFFER_SIZE 32
#define DRAW_SCORE(full_clip, game_clip, text, formatted_text, display, score) snprintf(text, BUFFER_SIZE, "SCORE: %03i", score);\
                    mbstowcs(formatted_text, text, BUFFER_SIZE);\
                    hagl_put_text(display_handle, formatted_text, game_clip.x0,\
                    clip.y0+(game_clip.y0-clip.y0)-SCORE_FONT.size_y-1, hagl_color(display_handle,255,255,255),\
                    SCORE_FONT.font_data);



#define INITIAL_SNAKE_SIZE 4
#define DRAW_TILE(pos_x, pos_y, size, clip, color) hagl_fill_rectangle_xywh(display_handle, pos_x*size+clip.x0, pos_y*size+clip.y0, size, size, color);
#define DRAW_TILE_WITH_OUTLINE(pos_x, pos_y, size, clip, color, outline_color) DRAW_TILE(pos_x, pos_y, size, clip, color) hagl_draw_rectangle_xywh(display_handle, pos_x*size+clip.x0, pos_y*size+clip.y0, tile_size, tile_size, outline_color);

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
    } // Change position of head according to move direction
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
            } // Change move direction CCW
            break;
        
        case SCROLL_UP_EVENT:
            current_direction = static_cast<direction_t>(current_direction + 1);
            if (current_direction == DIRECTION_PADDING_UPPER)
            {
                current_direction = static_cast<direction_t>(DIRECTION_PADDING_LOWER + 1);
            } // Change move direction CW
            break;

        default:
            break;
        }
    return current_direction;
}

void draw_string(char* str, wchar_t* formatted_str, int x, int y, hagl_backend_t* display, hagl_color_t color, font input_font) 
{
    mbstowcs(formatted_str, str, BUFFER_SIZE);
    hagl_put_text(display, formatted_str, x, y, color, input_font.font_data);
}

void draw_eyes(hagl_backend_t* display_handle, std::vector<coordinates>* segments, int tile_size, hagl_window_t clip, hagl_color_t eye_color, direction_t direction)
{
    // AWFUL!!!!!!!!!!!
    coordinates head_pos{.x=(*segments).back().x, .y=(*segments).back().y};
    
    if (direction == DIRECTION_UP || direction == DIRECTION_DOWN)
    {
        coordinates eye_pos{.x=head_pos.x*tile_size+clip.x0+1, .y=head_pos.y*tile_size+clip.y0+tile_size/3};
        coordinates eye_pos_2{.x=(head_pos.x+1)*tile_size+clip.x0-tile_size/3-1, .y=head_pos.y*tile_size+clip.y0+tile_size/3};

        hagl_fill_rectangle_xywh(display_handle, eye_pos.x, eye_pos.y, tile_size/3, tile_size/3, eye_color);
        hagl_fill_rectangle_xywh(display_handle, eye_pos_2.x, eye_pos_2.y, tile_size/3, tile_size/3, eye_color);
    }

    else
    {
        coordinates eye_pos{.x=head_pos.x*tile_size+clip.x0+tile_size/3, .y=head_pos.y*tile_size+clip.y0+1};
        coordinates eye_pos_2{.x=head_pos.x*tile_size+clip.x0+tile_size/3, .y=(head_pos.y+1)*tile_size+clip.y0-tile_size/3-1};

        hagl_fill_rectangle_xywh(display_handle, eye_pos.x, eye_pos.y, tile_size/3, tile_size/3, eye_color);
        hagl_fill_rectangle_xywh(display_handle, eye_pos_2.x, eye_pos_2.y, tile_size/3, tile_size/3, eye_color);
    }
    
}

void clean_segment_lines(hagl_backend_t* display_handle, std::vector<coordinates>* segments, int tile_size, hagl_window_t clip, hagl_color_t snake_color)
{
    // ABSOLUTELY TERRIBLE!!!
    for (int i = (*segments).size()-1; i > (*segments).size()-3; i--)
    {
        coordinates current_seg={.x=(*segments)[i].x, .y=(*segments)[i].y};
        coordinates prev_seg={.x=(*segments)[i-1].x, .y=(*segments)[i-1].y};
        coordinates pos={.x=current_seg.x*tile_size+clip.x0, .y=current_seg.y*tile_size+clip.y0};

        if (current_seg.x - prev_seg.x == 1)
        {
            hagl_draw_vline_xyh(display_handle, pos.x, pos.y+1, tile_size-2, snake_color);
            hagl_draw_vline_xyh(display_handle, pos.x-1, pos.y+1, tile_size-2, snake_color);    
        }
        else if (current_seg.x - prev_seg.x == -1)
        {
            hagl_draw_vline_xyh(display_handle, pos.x+tile_size, pos.y+1, tile_size-2, snake_color);
            hagl_draw_vline_xyh(display_handle, pos.x+tile_size-1, pos.y+1, tile_size-2, snake_color);
        }
        else if (current_seg.y - prev_seg.y == 1)
        {
            hagl_draw_hline_xyw(display_handle, pos.x+1, pos.y, tile_size-2, snake_color);
            hagl_draw_hline_xyw(display_handle, pos.x+1, pos.y-1, tile_size-2, snake_color);
        }
        else
        {
            hagl_draw_hline_xyw(display_handle, pos.x+1, pos.y+tile_size, tile_size-2, snake_color);
            hagl_draw_hline_xyw(display_handle, pos.x+1, pos.y+tile_size-1, tile_size-2, snake_color);
        }
    }

    // Clear tail line
    coordinates current_seg = {.x=(*segments).front().x, .y=(*segments).front().y};
    coordinates next_seg = {.x=(*segments)[1].x, .y=(*segments)[1].y};
    coordinates pos={.x=current_seg.x*tile_size+clip.x0, .y=current_seg.y*tile_size+clip.y0};

    if (current_seg.x - next_seg.x == 1)
    {
        hagl_draw_vline_xyh(display_handle, pos.x, pos.y+1, tile_size-2, snake_color);
    }
    else if (current_seg.x - next_seg.x == -1)
    {
        hagl_draw_vline_xyh(display_handle, pos.x+tile_size-1, pos.y+1, tile_size-2, snake_color);
    }
    else if (current_seg.y - next_seg.y == 1)
    {
        hagl_draw_hline_xyw(display_handle, pos.x+1, pos.y, tile_size-2, snake_color);
    }
    else
    {
        hagl_draw_hline_xyw(display_handle, pos.x+1, pos.y+tile_size-1, tile_size-2, snake_color);
    }
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

    for (int i = 1; i < (*segments).size()-2; i++)
    {
        coordinates segment_pos{.x=(*segments)[i].x, .y=(*segments)[i].y};
        if (head_pos.x == segment_pos.x && head_pos.y == segment_pos.y)
        {
            return true; // Return true if there was a collision
        }
    } // Check all segments for colission except the tail since this is checked before sorting the segments

    return false; // Return false if no collision detected
}

hagl_window_t get_game_clip(hagl_window_t clip, uint16_t* tile_size_ptr)
{
    uint16_t tile_size = ((clip.x1-clip.x0)/GRID_WIDTH)*GRID_HEIGHT<(clip.y1-clip.y0) ? 
    (clip.x1-clip.x0)/GRID_WIDTH : (clip.y1-clip.y0)/GRID_HEIGHT;

    uint16_t clip_offset_x = (clip.x1-clip.x0)-GRID_WIDTH*tile_size; // Offset of the pixles that can't be fitted inside a tile
    uint16_t clip_offset_y = (clip.y1-clip.y0)-GRID_HEIGHT*tile_size;

    if (clip_offset_x < 2)
    {
        tile_size = ((clip.x1-clip.x0 - 2 + clip_offset_x)/GRID_WIDTH)*GRID_HEIGHT<(clip.y1-clip.y0) ? 
        (clip.x1-clip.x0 - 2 + clip_offset_x)/GRID_WIDTH : (clip.y1-clip.y0)/GRID_HEIGHT;
        clip_offset_x = (clip.x1-clip.x0)-GRID_WIDTH*tile_size; // Offset of the pixles that can't be fitted inside a tile
        clip_offset_y = (clip.y1-clip.y0)-GRID_HEIGHT*tile_size;
    }// Making sure there is enough space to draw the boundrary

    // Making sure there is enough space above the playable area to draw text
    if (clip_offset_y < SCORE_FONT.size_y+2)
    {
        tile_size = ((clip.x1-clip.x0)/GRID_WIDTH)*GRID_HEIGHT<(clip.y1-clip.y0 - SCORE_FONT.size_y+2 + clip_offset_y) ? 
                    (clip.x1-clip.x0)/GRID_WIDTH : (clip.y1-clip.y0 - SCORE_FONT.size_y+2 + clip_offset_y)/GRID_HEIGHT;
        clip_offset_x = (clip.x1-clip.x0)-GRID_WIDTH*tile_size;
        clip_offset_y = (clip.y1-clip.y0)-GRID_HEIGHT*tile_size;
    }

    hagl_window_t final_clip{.x0= static_cast<uint16_t>(clip.x0+ clip_offset_x / 2),
                            .y0= static_cast<uint16_t>(clip.y0 + clip_offset_y - 1),
                            .x1= static_cast<uint16_t>(clip.x1 - clip_offset_x / 2),
                            .y1= static_cast<uint16_t>(clip.y1 - 2)}; // Actual playable area

    *tile_size_ptr = tile_size;
    return final_clip;

}

snake_game_widget::snake_game_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex, hagl_window_t clip) :
                                    widget(widget_display, display_mutex, clip)
{
    widget::requires_inputs = true;
    xTaskCreate(call_run_widget, "test_widget", 4096, this, 3, &task_handle); // Create task to call run_widget
}

snake_game_widget::~snake_game_widget()
{}

void snake_game_widget::run_widget()
{   
    uint16_t tile_size;

    hagl_window_t game_clip = get_game_clip(clip, &tile_size);

    int score = 0;
    uint16_t delay_amount = BASE_REFRESH_RATE;
    char text[BUFFER_SIZE];
    wchar_t formatted_text[BUFFER_SIZE]; // Formatted text to work with hagl library
    direction_t move_direction = DIRECTION_RIGHT;

    hagl_color_t snake_color = hagl_color(display_handle, 41, 233, 99);
    hagl_color_t outline_color = hagl_color(display_handle, 255, 55, 169);
    hagl_color_t food_color = hagl_color(display_handle, 0, 0, 255);
    hagl_color_t background_color = hagl_color(display_handle, 0, 0, 0);
    hagl_color_t text_color = hagl_color(display_handle, 255, 255 ,255);

    bool is_game_over = false;
    bool new_segment_created = false;
    coordinates food;
    coordinates head;

    std::vector<coordinates> snake_segments;
    for (int i = 0; i < INITIAL_SNAKE_SIZE; i++)
    {
        snake_segments.push_back(head);
    } // Initialize snake body

game_setup:
    head = {.x = GRID_WIDTH/2-INITIAL_SNAKE_SIZE, .y = GRID_HEIGHT/2};
    for (int i = 0; i < INITIAL_SNAKE_SIZE; i++)
    {
        snake_segments[i]=head;
        head.x += 1;
    } // Set initial segment positions

    find_new_food_position(&snake_segments, &food);    

    START_DRAW(display_handle, clip, display_mutex)

        hagl_draw_rectangle_xyxy(display_handle, game_clip.x0-1, game_clip.y0-1, game_clip.x1,
        game_clip.y1+1, hagl_color(display_handle,255,255,255)); // Draw boundrary
        
        DRAW_SCORE(clip, game_clip, text, formatted_text, display_handle, score);
        
        for (int i = 0; i < snake_segments.size(); i++)
        {
            DRAW_TILE_WITH_OUTLINE(snake_segments[i].x, snake_segments[i].y, tile_size, game_clip, snake_color, outline_color);
        } // Draw snake segments

        clean_segment_lines(display_handle, &snake_segments, tile_size, game_clip, snake_color);
        DRAW_TILE(food.x, food.y, tile_size, game_clip, food_color); // Draw food

    END_DRAW(display_mutex)
    
    while (1)
    {
        if (input_event == SHORT_PRESS_EVENT)
        {
            // Reset game stats
            is_game_over = false;
            input_event = INIT_EVENT;
            move_direction = DIRECTION_RIGHT;
            delay_amount = BASE_REFRESH_RATE;
            score = 0;

            DRAW_TILE(snake_segments.front().x, snake_segments.front().y, tile_size, game_clip, background_color); // Erase prev tail position

            // Shrink snake to initial size
            snake_segments.resize(INITIAL_SNAKE_SIZE);

            // Erase game screen
            START_DRAW(display_handle, game_clip, display_mutex)
                hagl_fill_rectangle_xyxy(display_handle, clip.x0, clip.y0, clip.x1, clip.y1, background_color);
            END_DRAW(display_mutex)
            
            goto game_setup;
        } // Restart game
        
        while (!is_game_over)
        {
            uint64_t time_at_start = esp_timer_get_time(); // Get time at start of iteration

            if(input_event!=INIT_EVENT)
            {
                move_direction = calculate_move_direction(move_direction, input_event);
                if (input_event == LONG_PRESS_EVENT)
                {
                    set_input_requirement(false);
                    is_game_over = true;
                    break;
                }
                
                input_event = INIT_EVENT; // Reset input
            }

            head.x = snake_segments.back().x; // Reference head position to sort the body later
            head.y = snake_segments.back().y;

            START_DRAW(display_handle, game_clip, display_mutex)

                if (snake_segments.back().x == food.x && snake_segments.back().y == food.y)
                {
                    coordinates new_segment{.x=snake_segments.back().x,.y=snake_segments.back().y};
                    snake_segments.push_back(new_segment);

                    find_new_food_position(&snake_segments, &food);
                    DRAW_TILE(food.x, food.y, tile_size, game_clip, food_color); // Draw food
                 
                    score++;
                    
                    SET_CLIP(display_handle, clip);
                    DRAW_SCORE(clip, game_clip, text, formatted_text, display_handle, score);
                    SET_CLIP(display_handle, game_clip);

                    if (score < SCORE_FOR_MAX_SPEED)
                    {
                        delay_amount -= (BASE_REFRESH_RATE - MAX_REFRESH_RATE)/SCORE_FOR_MAX_SPEED;
                    }
                    
                    new_segment_created = true;
                } // Check if the snake ate the food

                calculate_head_pos(&snake_segments, move_direction);

                if (snake_segments.back().x == GRID_WIDTH || snake_segments.back().x < 0 ||
                    snake_segments.back().y == GRID_HEIGHT || snake_segments.back().y < 0 ||
                    check_self_collisions(&snake_segments))
                {
                    is_game_over = true;
                    xSemaphoreGive(display_mutex); // Give back control of the display
                } // Check if snake crashed into the wall

                if (!new_segment_created)
                {
                    DRAW_TILE(snake_segments.front().x, snake_segments.front().y, tile_size, game_clip, background_color); // Erase prev tail position
                    sort_segments(&snake_segments, head);
                    DRAW_TILE_WITH_OUTLINE(snake_segments.front().x, snake_segments.front().y, tile_size, game_clip, snake_color, outline_color); // Draw snake head
                    hagl_draw_rectangle_xywh(display_handle, snake_segments.front().x*tile_size+game_clip.x0, snake_segments.front().y*tile_size+game_clip.y0, tile_size, tile_size, outline_color); // Draw tail outline to close snake shape
                } // Draw snake elements

                DRAW_TILE_WITH_OUTLINE(snake_segments.back().x, snake_segments.back().y, tile_size, game_clip, snake_color, outline_color) // Draw head
                DRAW_TILE_WITH_OUTLINE(snake_segments[snake_segments.size()-2].x, snake_segments[snake_segments.size()-2].y, tile_size, game_clip, snake_color, outline_color) // Clear previous segment

                clean_segment_lines(display_handle, &snake_segments, tile_size, game_clip, snake_color); // Clear leftover outlines inside the snake body
                draw_eyes(display_handle, &snake_segments, tile_size, game_clip, background_color, move_direction);
                new_segment_created = false;
                
                if (is_game_over)
                {
                    int text_x, text_y;
                    snprintf(text, BUFFER_SIZE, "GAME OVER!");
                    text_x = (clip.x1 - clip.x0 - strlen(text)*DIALOGUE_FONT.size_x)/2;
                    text_y = (clip.y1 - clip.y0)/2;
                    draw_string(text, formatted_text, text_x, text_y, display_handle, text_color, DIALOGUE_FONT);

                    snprintf(text, BUFFER_SIZE, "PLAY AGAIN?");
                    text_x = (clip.x1 - clip.x0 - strlen(text)*DIALOGUE_FONT.size_x)/2;
                    text_y = (clip.y1 - clip.y0)/2+DIALOGUE_FONT.size_y;
                    draw_string(text, formatted_text, text_x, text_y, display_handle, text_color, DIALOGUE_FONT);
                }
                
            END_DRAW(display_mutex)
            
            #ifdef CHECK_TASK_SIZES
            ESP_LOGI("clock_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
            #endif

            uint32_t iteration_time = (esp_timer_get_time() - time_at_start)/10000;
            CHECK_WIDGET_DELETION_STATUS(task_deletion_mutex)
            if (iteration_time < delay_amount)
            {
                vTaskDelay(pdMS_TO_TICKS(delay_amount-iteration_time));
            }

            else
            {
                vTaskDelay(pdMS_TO_TICKS(delay_amount));
            }
            
        }
    vTaskDelay(pdMS_TO_TICKS(200));
    } 
}