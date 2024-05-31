#include "apps.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fonts.h"

#include "hagl_hal.h"
#include "hagl.h"

#include <cstdio>
#include <cstring>
#include <ctime>

// #define CHECK_TASK_SIZES

/*
Apps:
Typical smart watches have faces/screens that show different sets of data/settings/etc and the user can switch between them.
My idea for an implimentation of such thing is having a template class from which all these screens will be derived
and will implement their own logic as well as their own functions to draw data/gui on the screen.

Guidelines:
1.  The app class should have a few basic functions:
    1.  Draw on screen - All the ui of the app should be handled by a function that draws the ui elements
    2.  Get data - Get all the data necessary for the app to work correctly
    3.  Check update conditions - A function to to check if the arguments needed by the app have changed from
        the previous iteration. This is done to decide if the draw on screen function needs to be called.
        (Less unecessary updating -> more optimized)

2.  Every app must have a few attributes:
        1.  Data required by the app to function (e.g, heart rate for a heart rate monitor app).
        2.  Reference of said data to check if the the gui needs to be updated.
        May be useful(?):    
        3.  Allocated area on screen - The bounding box of the app to be displayed in (might be useful for things like
            widgets (e.g, a bar on top to show the battery precentage and time))
        4.  Draw priority - When a few apps are running simultaneously, which ones should be drawn on top of which? 

3.  Apps should be as modular and flexible as possible. I'm hoping that my idea will serve as a solid template
    for anything from simple information displays, settings menus, widgets to a freaking game of Snake.
*/

extern "C" app::app(hagl_backend_t* display) : display_handle(display){};
extern "C" app::~app(){}
extern "C" void app::run_app(){}
extern "C" bool app::get_update_status(){return true;}

void inline call_run_app(void* app_obj)
{
    /*FreeRTOS expects a pointer to a function. C++ member functions, however are not pointers to a function
    and there is no way to convert them to that. Therefore, this function calls the member function itself when freertos calls it.*/
    static_cast<app*>(app_obj)->run_app();
}


extern "C" clock_app::clock_app(tm* time, hagl_backend_t* app_display)
: app(app_display), reference_time(*time), current_time(time)
{
    xTaskCreate(call_run_app, "clock_app", 10000, this, 3, &task_handle); // Create task to call run_app
}

extern "C" clock_app::~clock_app()
{
    hagl_clear(display_handle);
    vTaskDelete(task_handle);
}

extern "C" void clock_app::month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer
    (Assuming month range is 1-12)*/
    const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};    
    strcpy(buffer, months_of_year[month-1]); // Copy the month string into the buffer
}

extern "C" void clock_app::weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer
    (Assuming weekday range is 1-7)*/
    const char* days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday-1]); // Copy the day of week string into the buffer
}

extern "C" bool clock_app::get_update_status()
{   
    /*Update the internal reference and current times and return true if they were updated*/

    // mktime assumes tm_year is years since 1900.
    // It will also automatically calculate the weekday and override the original value.
    // Therefore tm needs to be modified before passing it
    reference_time.tm_year -= 1900;
    tm current_time_struct = *current_time;
    current_time_struct.tm_year -= 1900;

    if (difftime(mktime(&reference_time), mktime(&current_time_struct)) < 0) 
    {
        reference_time.tm_year += 1900;
        current_time_struct.tm_year += 1900;
        reference_time = current_time_struct;
        return true;
    } // Check if the time got iterated since last run and update accordingly
    reference_time.tm_year += 1900;
    current_time_struct.tm_year += 1900;
    return false; // Return false if time was not iterated
}

extern "C" void clock_app::run_app ()
{
    static const TickType_t task_delay_ms = pdMS_TO_TICKS(1000);
    static const hagl_color_t color = hagl_color(display_handle, 255, 100, 255); // Placeholder color (Note: the r and b channels are inverted on my display)
    
    while (1)
    {
        tm current_time_struct = *current_time;

        /*Display hours*/
        if (current_time_struct.tm_hour != reference_time.tm_hour || first_run)
        {
            snprintf(time_str, 64, "%02i", current_time_struct.tm_hour);
            hours_text_cords_y = 20;
            mbstowcs(formatted_str, time_str, 64);
            hours_text_cords_y = segment_font.size_y;
            hagl_put_text(display_handle, formatted_str, (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2, hours_text_cords_y, color, segment_font.font);
        }

        /*Display minutes*/
        if (current_time_struct.tm_min != reference_time.tm_min || first_run)
        {
            snprintf(time_str, 64, "%02i", current_time_struct.tm_min);
            mbstowcs(formatted_str, time_str, 64);
            minutes_text_cords_y = hours_text_cords_y + 5 + segment_font.size_y;
            hagl_put_text(display_handle, formatted_str, (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2, minutes_text_cords_y, color, segment_font.font); // Display string
        }
        
        /*Display seconds*/
        snprintf(time_str, 64, "%02i", current_time_struct.tm_sec);
        mbstowcs(formatted_str, time_str, 64);
        seconds_text_cords_y = minutes_text_cords_y + 5 + segment_font.size_y;
        hagl_put_text(display_handle, formatted_str, (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2, seconds_text_cords_y, color, segment_font.font); // Display string
        
        /*Display month day, month name and year*/
        if (current_time_struct.tm_mday != reference_time.tm_mday || first_run)
        {
            month_to_str(current_time_struct.tm_mon, month);
            snprintf(time_str, 64, "%02i %s %04i", 
            current_time_struct.tm_mday, month, current_time_struct.tm_year);
            mbstowcs(formatted_str, time_str, 64);
            months_text_cords_y = seconds_text_cords_y + 5 + segment_font.size_y;
            hagl_put_text(display_handle, formatted_str, DISPLAY_WIDTH/2 - strlen(time_str)*4, months_text_cords_y, color, font10x20.font); // Display string
        }

        /*Display weekday*/
        if (current_time_struct.tm_wday != reference_time.tm_wday || first_run)
        {
            weekday_to_str(current_time_struct.tm_wday, weekday);
            wchar_t formatted_weekday[10];
            mbstowcs(formatted_weekday, weekday, 10);
            weekday_text_cords_y = months_text_cords_y + 5 + font10x20.size_y;
            hagl_put_text(display_handle, formatted_weekday, DISPLAY_WIDTH/2 - strlen(weekday)*4, weekday_text_cords_y, color, font10x20.font); // Display string
        }
        
        reference_time = current_time_struct;
        if (first_run)
        {
            first_run = false;
        }
        
    #ifdef CHECK_TASK_SIZES
        ESP_LOGI(TAG, "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
    #endif
    vTaskDelay(task_delay_ms);

    }
}

/*test_app for testing!*/

extern "C" test_app::test_app(hagl_backend_t* app_display) : app(app_display)
{
    xTaskCreate(call_run_app, "test_app", 10000, this, 3, &task_handle); // Create task to call run_app
}

extern "C" test_app::~test_app()
{
    hagl_clear(display_handle);
    vTaskDelete(task_handle);
}

extern "C" bool test_app::get_update_status()
{
    return true;
}

extern "C" void test_app::run_app()
{
    static const TickType_t delay = 250 / portTICK_PERIOD_MS;
    // static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);
    int radius = 1;
    while (1)
    {
        hagl_draw_circle(display_handle, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, radius, (hagl_color_t)16379);
        radius += 1;
        vTaskDelay(delay);
    }
}