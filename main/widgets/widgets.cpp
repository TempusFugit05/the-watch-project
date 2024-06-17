#include "widgets.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <ctime>
#include <random>

#include "ds3231/ds3231.h"

// #define CHECK_TASK_SIZES

widget::widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex) : display_handle(display), display_mutex(display_mutex){};
widget::~widget(){}
void widget::run_widget(){}

input_event_t widget::set_input_event(input_event_t event)
{
    input_event = event;
    return event;
}

bool widget::get_input_requirements()
{
    return requires_inputs;
}


/*test_widget for testing!*/

test_widget::test_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex) : widget(widget_display, display_mutex)
{
    xTaskCreate(call_run_widget, "test_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

test_widget::~test_widget()
{
    if (xSemaphoreTake(display_mutex, portMAX_DELAY))
    {
        vTaskDelete(task_handle);
        hagl_set_clip(display_handle,0 ,20, DISPLAY_WIDTH, DISPLAY_HEIGHT); // Set drawable area
        hagl_fill_rectangle_xyxy(display_handle, 0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT, hagl_color(display_handle,0,0,0)); // Clear screen
        xSemaphoreGive(display_mutex);
    } // Clear screen
    
}
void test_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);
    int radius = 0;
    while (1)
    {
        if (xSemaphoreTake(display_mutex, pdMS_TO_TICKS(portMAX_DELAY)))
        {
            hagl_set_clip(display_handle,0 ,20, DISPLAY_WIDTH, DISPLAY_HEIGHT); // Set drawable area
            hagl_draw_circle(display_handle, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, radius, color);
            radius += 1;

        #ifdef CHECK_TASK_SIZES
        ESP_LOGI("test_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
            xSemaphoreGive(display_mutex);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
