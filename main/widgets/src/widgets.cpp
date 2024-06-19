#include "widgets.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <ctime>
#include <random>

#include "ds3231/ds3231.h"

// #define CHECK_TASK_SIZES

widget::widget(hagl_backend_t* display, SemaphoreHandle_t display_mutex, hagl_window_t clip) :
display_handle(display), display_mutex(display_mutex), clip(clip)
{
    task_deletion_mutex = xSemaphoreCreateMutex();
}

widget::~widget()
{
}
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

bool widget::set_input_requirement(bool requirement_state)
{
    requires_inputs = requirement_state;
    return requirement_state;
}

/*test_widget for testing!*/

test_widget::test_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex, hagl_window_t clip) : widget(widget_display, display_mutex, clip)
{
    xTaskCreate(call_run_widget, "test_widget", 2048, this, 3, &task_handle); // Create task to call run_widget
}

test_widget::~test_widget()
{
    START_WIDGET_DELETION(task_deletion_mutex)
    CLEAR_SCREEN(display_handle, clip, display_mutex)
    END_WIDGET_DELETION(task_deletion_mutex)
}

void test_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);
    int radius = 0;
    while (1)
    {
        START_DRAW(display_handle, clip, display_mutex)
            hagl_draw_circle(display_handle, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, radius, color);
        END_DRAW(display_mutex)
            radius += 1;

        #ifdef CHECK_TASK_SIZES
        ESP_LOGI("test_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif

            CHECK_WIDGET_DELETION_STATUS(task_deletion_mutex)
            
            vTaskDelay(pdMS_TO_TICKS(10));
    }
}
