#include "widgets.h"
#include "fonts.h"
#include <cstring>

clock_widget::clock_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex, ds3231_handle_t* rtc)
: widget(widget_display, display_mutex), rtc_handle(rtc)
{
    ds3231_get_datetime(rtc_handle, &reference_time);
    xTaskCreate(call_run_widget, "clock_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

clock_widget::~clock_widget()
{
    if (xSemaphoreTake(display_mutex, pdMS_TO_TICKS(portMAX_DELAY)))
    {
        vTaskDelete(task_handle);
        hagl_set_clip(display_handle,0 ,20, DISPLAY_WIDTH, DISPLAY_HEIGHT); // Set drawable area
        hagl_fill_rectangle_xyxy(display_handle, 0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT, hagl_color(display_handle,0,0,0)); // Clear screen
        xSemaphoreGive(display_mutex);
    } // Clear screen
}

void clock_widget::month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer*/
    static const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};    
    strcpy(buffer, months_of_year[month]); // Copy the month string into the buffer
}

void clock_widget::weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer*/
    static const char* days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday]); // Copy the day of week string into the buffer
}

void clock_widget::run_widget ()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 100, 255); // Placeholder color (Note: the r and b channels are inverted on my display)
    
    while (1)
    {
        if (xSemaphoreTake(display_mutex, portMAX_DELAY))
        {
        
            hagl_set_clip(display_handle,0 ,20, DISPLAY_WIDTH, DISPLAY_HEIGHT); // Set drawable area
            struct tm current_time;
            ds3231_get_datetime(rtc_handle, &current_time);

            /*Display hours*/
            if (current_time.tm_hour != reference_time.tm_hour || first_run)
            {
                snprintf(time_str, 64, "%02i", current_time.tm_hour);
                hours_text_cords_y = 20;
                mbstowcs(formatted_str, time_str, 64);
                hours_text_cords_y = segment_font.size_y;
                hagl_put_text(display_handle, formatted_str, (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2, hours_text_cords_y, color, segment_font.font);
            }
            
            /*Display minutes*/
            if (current_time.tm_min != reference_time.tm_min || first_run)
            {
                snprintf(time_str, 64, "%02i", current_time.tm_min);
                mbstowcs(formatted_str, time_str, 64);
                minutes_text_cords_y = hours_text_cords_y + 5 + segment_font.size_y;
                hagl_put_text(display_handle, formatted_str, (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2, minutes_text_cords_y, color, segment_font.font); // Display string
            }
            
            /*Display seconds*/
            snprintf(time_str, 64, "%02i", current_time.tm_sec);
            mbstowcs(formatted_str, time_str, 64);
            seconds_text_cords_y = minutes_text_cords_y + 5 + segment_font.size_y;
            hagl_put_text(display_handle, formatted_str, (DISPLAY_WIDTH - strlen(time_str)*segment_font.size_x)/2, seconds_text_cords_y, color, segment_font.font); // Display string
            
            /*Display month day, month name and year*/
            if (current_time.tm_mday != reference_time.tm_mday || first_run)
            {
                current_time.tm_year += 1900;
                month_to_str(current_time.tm_mon, month_str);
                snprintf(time_str, 64, "%02i %s %04i", 
                current_time.tm_mday, month_str, current_time.tm_year);
                mbstowcs(formatted_str, time_str, 64);
                months_text_cords_y = seconds_text_cords_y + 5 + segment_font.size_y;
                hagl_put_text(display_handle, formatted_str, DISPLAY_WIDTH/2 - strlen(time_str)*4, months_text_cords_y, color, font10x20.font); // Display string
            }

            /*Display weekday*/
            if (current_time.tm_wday != reference_time.tm_wday || first_run)
            {
                weekday_to_str(current_time.tm_wday, weekday_str);
                mbstowcs(formatted_weekday, weekday_str, 10);
                weekday_text_cords_y = months_text_cords_y + 5 + font10x20.size_y;
                hagl_put_text(display_handle, formatted_weekday, DISPLAY_WIDTH/2 - strlen(weekday_str)*4, weekday_text_cords_y, color, font10x20.font); // Display string
            }
            
            reference_time = current_time;
            if (first_run)
            {
                first_run = false;
            }
            xSemaphoreGive(display_mutex);
        #ifdef CHECK_TASK_SIZES
            ESP_LOGI("clock_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
