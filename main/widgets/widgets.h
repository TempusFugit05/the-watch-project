#ifndef WIDGETS_H
#define WIDGETS_H

#include <ctime>

#include "./hagl_hal.h"
#include "./hagl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds3231.h"
#include "grid/grid.h"

#define SCREEN_SIZE_X 240
#define SCREEN_SIZE_Y 240

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

void inline call_run_widget(void* widget_obj)
{
    /*FreeRTOS expects a pointer to a function. C++ member functions, however are not pointers to a function
    and there is no way to convert them to that. Therefore, this function calls the member function itself when freertos calls it.*/
    static_cast<widget*>(widget_obj)->run_widget();
}

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
        
        int tile_size_x;
        int tile_size_y;

        virtual_grid* game_grid;

        void draw_tile_map();
        
    public:
        snake_game_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t display_bounds);
        ~snake_game_widget() override;
        void run_widget() override;
};

#endif // WIDGETS_