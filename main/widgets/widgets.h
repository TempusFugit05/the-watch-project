#ifndef WIDGETS_H
#define WIDGETS_H

#include <ctime>

#include "./hagl_hal.h"
#include "./hagl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds3231.h"
#include "input_event_types.h"

/*
Widgets:
Typical smart watches have faces/screens that show different sets of data/settings/etc and the user can switch between them.
My idea for an implimentation of such thing is having a template class from which all these screens will be derived
and will implement their own logic as well as their own functions to draw data/gui on the screen.

Guidelines:
1.  The widget class should have a few basic functions:
    1.  Draw on screen - All the ui of the widget should be handled by a function that draws the ui elements
    2.  Get data - Get all the data necessary for the widget to work correctly

2.  Every widget must have a few attributes:
        1.  Data required by the widget to function (e.g, heart rate for a heart rate monitor widget).
        2.  Reference of said data to check if the the gui needs to be updated.
        May be useful(?):    
        3.  Allocated area on screen - The bounding box of the widget to be displayed in (might be useful for things like
            widgets (e.g, a bar on top to show the battery precentage and time))
        4.  Draw priority - When a few widgets are running simultaneously, which ones should be drawn on top of which? 

3.  Widgets should be as modular and flexible as possible. I'm hoping that my idea will serve as a solid template
    for anything from simple information displays, settings menus, widgets to a freaking game of Snake.
*/

class widget
{    
protected:
    hagl_backend_t* display_handle;
    SemaphoreHandle_t display_mutex;
    TaskHandle_t task_handle;
    bool requires_inputs = false;
    input_event_t input_event = INIT_EVENT;

public:
    widget(hagl_backend_t* display_handle, SemaphoreHandle_t  display_mutex);
    input_event_t set_input_event(input_event_t event);
    bool get_input_requirements();
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
        hagl_window_t clip;

        char time_str[32];
        wchar_t formatted_time_str[32];
        bool first_run = true;
        
    public:
        info_bar_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, ds3231_handle_t* rtc, hagl_window_t clip);
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

        void draw_tile_map();
        
    public:
        snake_game_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t clip);
        ~snake_game_widget() override;
        void run_widget() override;
};

#endif // WIDGETS_