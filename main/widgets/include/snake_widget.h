#ifndef SNAKE_WIDGET_H
#define SNAKE_WIDGET_H

#include "widget.h"
#include <vector>

typedef enum : uint8_t 
{
    DIRECTION_PADDING_LOWER,
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_PADDING_UPPER,
}direction_t;

class snake_game_widget : public widget
{
    private:

    uint16_t tile_size;

    hagl_window_t game_clip;

    int score = 0;
    uint16_t delay_amount;
    direction_t move_direction = DIRECTION_RIGHT;

    hagl_color_t snake_color = hagl_color(display_handle, 41, 233, 99);
    hagl_color_t outline_color = hagl_color(display_handle, 255, 55, 169);
    hagl_color_t food_color = hagl_color(display_handle, 0, 0, 255);
    hagl_color_t background_color = hagl_color(display_handle, 0, 0, 0);
    hagl_color_t text_color = hagl_color(display_handle, 255, 255 ,255);

    std::vector<coordinates> snake_segments;
    bool is_game_over = false;
    bool new_segment_created = false;
    bool first_run = true;
    coordinates food;
    coordinates head;
    
    void reset_game();

    public:
        snake_game_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t clip);
        ~snake_game_widget() override;
        void run_widget() override;
};

#endif // SNAKE_WIDGET_H