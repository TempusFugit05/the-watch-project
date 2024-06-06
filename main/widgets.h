#ifndef WIDGETS_H
#define WIDGETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ctime>

#include "hagl_hal.h"
#include "hagl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds3231.h"

class widget
{    
protected:
    hagl_backend_t* display_handle;
    SemaphoreHandle_t display_mutex;
    TaskHandle_t task_handle;

public:
    widget(hagl_backend_t* display_handle, SemaphoreHandle_t  display_mutex);
    virtual ~widget() = 0;
    virtual void run_widget() = 0;
};

class clock_widget : public widget
{
    private:
        tm reference_time; // Older time to compare against
        ds3231_handle_t* rtc_handle;

        char time_str [64]; // Buffer to display the time 
        wchar_t formatted_str [64]; // String buffer compatable with the display library
        char month_str[10]; // Buffer to store the month's name
        
        char weekday_str[10]; // Buffer to store the week's name
        wchar_t formatted_weekday[10];
        
        void month_to_str(int month, char* buffer);
        void weekday_to_str(int weekday, char* buffer);
        
        bool first_run = true;

        int hours_text_cords_y;
        int minutes_text_cords_y;
        int seconds_text_cords_y;
        int months_text_cords_y;
        int weekday_text_cords_y;

    public:
        clock_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, ds3231_handle_t* rtc);
        ~clock_widget() override;
        void run_widget () override;

};

class info_bar_widget : public widget
{
    private:
        tm reference_time;
        ds3231_handle_t* rtc_handle;

        char time_str[32];
        wchar_t formatted_time_str[32];
        bool first_run = true;
        
    public:
        info_bar_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, ds3231_handle_t* rtc);
        ~info_bar_widget() override;
        void run_widget() override;
};

class test_widget : public widget
{
    public:
        test_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex);
        ~test_widget() override;
        void run_widget() override;
};

class snake_game_widget : public widget
{
    private:
        hagl_window_t clip;
        
        struct tile
        {
            uint16_t x;
            uint16_t y;
            hagl_color_t color;
        };
        
        int tile_size_x;
        int tile_size_y;

        int** game_tiles;
        int num_tiles;

        tile** background_tiles;
        void create_background();
        void draw_background();

    public:
        snake_game_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t display_bounds);
        ~snake_game_widget() override;
        void run_widget() override;
};

#ifdef __cplusplus
}
#endif

#endif // WIDGETS_