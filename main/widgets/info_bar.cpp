#include "widgets.h"
#include "./font6x9.h"
#include <cstring>

info_bar_widget::info_bar_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, ds3231_handle_t* rtc) : widget(display, display_mutex), rtc_handle(rtc)
{
    ds3231_get_datetime(rtc_handle, &reference_time);
    xTaskCreate(call_run_widget, "info_bar", 2048, this, 3, &task_handle); // Create task to call run_widget
}

info_bar_widget::~info_bar_widget()
{
    if (xSemaphoreTake(display_mutex, portMAX_DELAY))
    {
        hagl_set_clip(display_handle, 0 ,0, SCREEN_SIZE_X, 20); // Set drawable area
        vTaskDelete(task_handle);
        hagl_fill_rectangle_xyxy(display_handle, 0, 0, SCREEN_SIZE_X, 20, hagl_color(display_handle,0,0,0)); // Clear screen
        xSemaphoreGive(display_mutex);
    } // Clear screen
}

void info_bar_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);

    while (1)
    {
        if (xSemaphoreTake(display_mutex, pdMS_TO_TICKS(portMAX_DELAY)))
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
            xSemaphoreGive(display_mutex);

            #ifdef CHECK_TASK_SIZES
                ESP_LOGI("info_bar_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
            #endif
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
}
