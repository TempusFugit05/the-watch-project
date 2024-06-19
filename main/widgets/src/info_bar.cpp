#include "widget.h"
#include "info_bar_widget.h"

#include "fonts.h"
#include <cstring>
#include "esp_log.h"

#define FONT font6x9

info_bar_widget::info_bar_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t clip, ds3231_handle_t* rtc) : 
widget(display, display_mutex, clip, "info_bar_widget"), rtc_handle(rtc)
{
    ds3231_get_datetime(rtc_handle, &reference_time);
    xTaskCreate(call_run_widget, WIDGET_NAME, 2048, this, 3, &task_handle); // Create task to call run_widget
}

info_bar_widget::~info_bar_widget()
{   
    START_WIDGET_DELETION(task_handle, task_deletion_mutex);
    CLEAR_SCREEN(display_handle, clip, display_mutex);
}

void info_bar_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);

    while (1)
    {
            struct tm current_time;
            ds3231_get_datetime(rtc_handle, &current_time);

            if (reference_time.tm_sec != current_time.tm_sec || first_run)
            {
                snprintf(time_str, 32, "%02i:%02i:%02i",
                        current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
                mbstowcs(formatted_time_str, time_str, 32);
                
                START_DRAW(display_handle, clip, display_mutex);
                hagl_put_text(display_handle, formatted_time_str, (clip.x1-clip.x0-strlen(time_str)*FONT.size_x)/2, clip.y1/2, color, FONT.font_data);
                END_DRAW(display_mutex);
            }

            #ifdef CHECK_TASK_SIZES
                ESP_LOGI("info_bar_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
            #endif
            
        END_ITERATION(1000, task_handle, task_deletion_mutex);
    }
}
