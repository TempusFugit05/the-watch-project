#ifndef CLOCK_WIDGET_H
#define CLOCK_WIDGET_H

#include "widget.h"
#include "hagl.h"

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

        hagl_color_t text_color;
        
        bool first_run = true;

        int hours_text_cords_y;
        int minutes_text_cords_y;
        int seconds_text_cords_y;
        int months_text_cords_y;
        int weekday_text_cords_y;

        void display_hours(tm current_time);
        void display_minutes(tm current_time);
        void display_seconds(tm current_time);
        void display_date(tm current_time);
        void display_weekday(tm current_time);

    public:
        clock_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t clip, ds3231_handle_t* rtc);
        ~clock_widget() override;
        void run_widget () override;

};

#endif // CLOCK_WIDGET_H