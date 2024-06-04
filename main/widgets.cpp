#include "widgets.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "font6x9.h"
#include "fonts.h"

#include "hagl_hal.h"
#include "hagl.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "ds3231.h"

#define SCREEN_SIZE_X 240
#define SCREEN_SIZE_Y 240

// #define CHECK_TASK_SIZES

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

extern "C" widget::widget(hagl_backend_t* display, SemaphoreHandle_t  mutex) : display_handle(display), mutex(mutex){};
extern "C" widget::~widget(){}
extern "C" void widget::run_widget(){}

void inline call_run_widget(void* widget_obj)
{
    /*FreeRTOS expects a pointer to a function. C++ member functions, however are not pointers to a function
    and there is no way to convert them to that. Therefore, this function calls the member function itself when freertos calls it.*/
    static_cast<widget*>(widget_obj)->run_widget();
}


extern "C" clock_widget::clock_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  mutex, ds3231_handle_t* rtc)
: widget(widget_display, mutex), rtc_handle(rtc)
{
    ds3231_get_datetime(rtc_handle, &reference_time);
    xTaskCreate(call_run_widget, "clock_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

extern "C" clock_widget::~clock_widget()
{
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)))
    {
        vTaskDelete(task_handle);
        hagl_set_clip(display_handle,0 ,20, SCREEN_SIZE_X, SCREEN_SIZE_Y); // Set drawable area
        hagl_fill_rectangle_xyxy(display_handle, 0, 20, SCREEN_SIZE_X, SCREEN_SIZE_Y, hagl_color(display_handle,0,0,0)); // Clear screen
        xSemaphoreGive(mutex);
    } // Clear screen
}

extern "C" void clock_widget::month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer*/
    static const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};    
    strcpy(buffer, months_of_year[month]); // Copy the month string into the buffer
}

extern "C" void clock_widget::weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer*/
    static const char* days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday]); // Copy the day of week string into the buffer
}

extern "C" void clock_widget::run_widget ()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 100, 255); // Placeholder color (Note: the r and b channels are inverted on my display)
    
    while (1)
    {
        if (xSemaphoreTake(mutex, portMAX_DELAY))
        {
        
            hagl_set_clip(display_handle,0 ,20, SCREEN_SIZE_X, SCREEN_SIZE_Y); // Set drawable area
            struct tm current_time;
            ds3231_get_datetime(rtc_handle, &current_time);

            /*Display hours*/
            if (current_time.tm_hour != reference_time.tm_hour || first_run)
            {
                snprintf(time_str, 64, "%02i", current_time.tm_hour);
                hours_text_cords_y = 20;
                mbstowcs(formatted_str, time_str, 64);
                hours_text_cords_y = segment_font.size_y;
                hagl_put_text(display_handle, formatted_str, (SCREEN_SIZE_X - strlen(time_str)*segment_font.size_x)/2, hours_text_cords_y, color, segment_font.font);
            }
            /*Display minutes*/
            if (current_time.tm_min != reference_time.tm_min || first_run)
            {
                snprintf(time_str, 64, "%02i", current_time.tm_min);
                mbstowcs(formatted_str, time_str, 64);
                minutes_text_cords_y = hours_text_cords_y + 5 + segment_font.size_y;
                hagl_put_text(display_handle, formatted_str, (SCREEN_SIZE_X - strlen(time_str)*segment_font.size_x)/2, minutes_text_cords_y, color, segment_font.font); // Display string
            }
            
            /*Display seconds*/
            snprintf(time_str, 64, "%02i", current_time.tm_sec);
            mbstowcs(formatted_str, time_str, 64);
            seconds_text_cords_y = minutes_text_cords_y + 5 + segment_font.size_y;
            hagl_put_text(display_handle, formatted_str, (SCREEN_SIZE_X - strlen(time_str)*segment_font.size_x)/2, seconds_text_cords_y, color, segment_font.font); // Display string
            
            /*Display month day, month name and year*/
            if (current_time.tm_mday != reference_time.tm_mday || first_run)
            {
                current_time.tm_year += 1900;
                month_to_str(current_time.tm_mon, month_str);
                snprintf(time_str, 64, "%02i %s %04i", 
                current_time.tm_mday, month_str, current_time.tm_year);
                mbstowcs(formatted_str, time_str, 64);
                months_text_cords_y = seconds_text_cords_y + 5 + segment_font.size_y;
                hagl_put_text(display_handle, formatted_str, SCREEN_SIZE_X/2 - strlen(time_str)*4, months_text_cords_y, color, font10x20.font); // Display string
            }

            /*Display weekday*/
            if (current_time.tm_wday != reference_time.tm_wday || first_run)
            {
                weekday_to_str(current_time.tm_wday, weekday_str);
                mbstowcs(formatted_weekday, weekday_str, 10);
                weekday_text_cords_y = months_text_cords_y + 5 + font10x20.size_y;
                hagl_put_text(display_handle, formatted_weekday, SCREEN_SIZE_X/2 - strlen(weekday_str)*4, weekday_text_cords_y, color, font10x20.font); // Display string
            }
            
            reference_time = current_time;
            if (first_run)
            {
                first_run = false;
            }
            xSemaphoreGive(mutex);
        #ifdef CHECK_TASK_SIZES
            ESP_LOGI("clock_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/*info_bar_widget to display time and other info if*/

extern "C" info_bar_widget::info_bar_widget(hagl_backend_t* display, SemaphoreHandle_t  mutex, ds3231_handle_t* rtc) : widget(display, mutex), rtc_handle(rtc)
{
    ds3231_get_datetime(rtc_handle, &reference_time);
    xTaskCreate(call_run_widget, "info_bar", 2048, this, 3, &task_handle); // Create task to call run_widget
}

extern "C" info_bar_widget::~info_bar_widget()
{
    if (xSemaphoreTake(mutex, portMAX_DELAY))
    {
        hagl_set_clip(display_handle, 0 ,0, SCREEN_SIZE_X, 20); // Set drawable area
        vTaskDelete(task_handle);
        hagl_fill_rectangle_xyxy(display_handle, 0, 0, SCREEN_SIZE_X, 20, hagl_color(display_handle,0,0,0)); // Clear screen
        xSemaphoreGive(mutex);
    } // Clear screen
}

extern "C" void info_bar_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);

    while (1)
    {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)))
        {
            hagl_set_clip(display_handle, 0 ,0, SCREEN_SIZE_X, 20); // Set drawable area
            struct tm current_time;
            ds3231_get_datetime(rtc_handle, &current_time);

            if (reference_time.tm_sec != current_time.tm_sec || first_run)
            {
                snprintf(time_str, 32, "%02i:%02i:%02i",
                        current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
                mbstowcs(formatted_time_str, time_str, 32);
                hagl_put_text(display_handle, formatted_time_str, (SCREEN_SIZE_X-strlen(time_str)*5)/2, 10, color, font6x9);
            }
            xSemaphoreGive(mutex);

            #ifdef CHECK_TASK_SIZES
                ESP_LOGI("info_bar_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
            #endif
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
}

/*test_widget for testing!*/

extern "C" test_widget::test_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  mutex) : widget(widget_display, mutex)
{
    xTaskCreate(call_run_widget, "test_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

extern "C" test_widget::~test_widget()
{
    if (xSemaphoreTake(mutex, portMAX_DELAY))
    {
        vTaskDelete(task_handle);
        hagl_set_clip(display_handle,0 ,20, SCREEN_SIZE_X, SCREEN_SIZE_Y); // Set drawable area
        hagl_fill_rectangle_xyxy(display_handle, 0, 20, SCREEN_SIZE_X, SCREEN_SIZE_Y, hagl_color(display_handle,0,0,0)); // Clear screen
        xSemaphoreGive(mutex);
    } // Clear screen
}
extern "C" void test_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);
    int radius = 0;
    while (1)
    {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)))
        {
            hagl_set_clip(display_handle,0 ,20, SCREEN_SIZE_X, SCREEN_SIZE_Y); // Set drawable area
            hagl_draw_circle(display_handle, SCREEN_SIZE_X/2, SCREEN_SIZE_Y/2, radius, color);
            radius += 1;

        #ifdef CHECK_TASK_SIZES
        ESP_LOGI("test_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
            xSemaphoreGive(mutex);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}