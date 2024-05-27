#include "apps.h"
#include "esp_log.h"
#include "fonts/font10x20.h"
#include "fonts/test_font.h"
#include "hagl_hal.h"
#include "hagl.h"

#include <cstdio>
#include <cstring>
#include <ctime>

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


extern "C" clock_app::clock_app(tm initial_time, bounding_box_t bounding_box_config, hagl_backend_t* app_display)
: reference_time(initial_time), current_time(initial_time), bounding_box(bounding_box_config), display_handle(app_display){}

extern "C" void clock_app::month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer
    (Assuming month range is 1-12)*/
    const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};    
    strcpy(buffer, months_of_year[month-1]); // Copy the month string into the buffer (including null terminator)
}

extern "C" void clock_app::weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer
    (Assuming weekday range is 1-7)*/
    const char* days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday-1]); // Copy the day of week string into the buffer (including null terminator)
}

extern "C" void clock_app::update_time(tm time)
{
    current_time = time;
}

extern "C" bool clock_app::get_update_status()
{   
    /*Update the internal reference and current times and return true if they were updated*/

    // mktime assumes tm_year is years since 1900.
    // It will also automatically calculate the weekday and override the original value.
    // Therefore tm needs to be modified before passing it
    reference_time.tm_year -= 1900;
    current_time.tm_year -= 1900;

    if (difftime(mktime(&reference_time), mktime(&current_time)) < 0) 
    {
        reference_time.tm_year += 1900;
        current_time.tm_year += 1900;
        reference_time = current_time;
        return true;
    } // Check if the time got iterated since last run and update accordingly
    reference_time.tm_year += 1900;
    current_time.tm_year += 1900;
    return false; // Return false if time was not iterated
}

extern "C" void clock_app::run_app ()
{
    static hagl_color_t color = hagl_color(display_handle, 255, 100, 255); // Placeholder color (Note: the r and b channels are inverted on my display)

    snprintf(time_str, 64, "%02i", current_time.tm_hour);
    int text_cords_x = (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2;
    int text_cords_y = 20 + segment_font.size_y;
    mbstowcs(formatted_str, time_str, 64);
    hagl_put_text(display_handle, formatted_str, text_cords_x, 40, color, segment_font.font); // Display string

    snprintf(time_str, 64, "%02i", current_time.tm_min);
    text_cords_x = (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2;
    text_cords_y += segment_font.size_y + 10;
    mbstowcs(formatted_str, time_str, 64);
    hagl_put_text(display_handle, formatted_str, text_cords_x, 85, color, segment_font.font); // Display string

    snprintf(time_str, 64, "%02i", current_time.tm_sec);
    text_cords_x = (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2;
    text_cords_y += segment_font.size_y + 10;
    mbstowcs(formatted_str, time_str, 64);
    hagl_put_text(display_handle, formatted_str, text_cords_x, 130, color, segment_font.font); // Display string
    
    char month[10]; // Buffer to store the month's name
    month_to_str(current_time.tm_mon, month);
    snprintf(time_str, 64, "%02i %s %04i", 
    current_time.tm_mday, month, current_time.tm_year);
    mbstowcs(formatted_str, time_str, 64);
    hagl_put_text(display_handle, formatted_str, DISPLAY_WIDTH/2 - strlen(time_str)*4, 200, color, font10x20); // Display string

    char weekday[10];
    weekday_to_str(current_time.tm_wday, weekday);
    wchar_t formatted_weekday[10];
    mbstowcs(formatted_weekday, weekday, 10);
    hagl_put_text(display_handle, formatted_weekday, DISPLAY_WIDTH/2 - strlen(weekday)*4, 220, color, font10x20); // Display string
}