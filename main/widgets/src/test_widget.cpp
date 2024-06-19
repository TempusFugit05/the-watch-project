#include "widget.h"
#include "test_widget.h"

test_widget::test_widget(hagl_backend_t* widget_display, SemaphoreHandle_t  display_mutex, hagl_window_t clip) : 
widget(widget_display, display_mutex, clip, "test_widget")
{
    xTaskCreate(call_run_widget, WIDGET_NAME, 2048, this, 3, &task_handle); // Create task to call run_widget
}

test_widget::~test_widget()
{
    START_WIDGET_DELETION(task_handle, task_deletion_mutex);
    CLEAR_SCREEN(display_handle, clip, display_mutex);
}

void test_widget::run_widget()
{
    static const hagl_color_t color = hagl_color(display_handle, 255, 255, 255);
    int radius = 0;
    while (1)
    {
        START_DRAW(display_handle, clip, display_mutex);
        hagl_draw_circle(display_handle, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, radius, color);
        END_DRAW(display_mutex);
        radius += 1;

        #ifdef CHECK_TASK_SIZES
        ESP_LOGI("test_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif

            
        END_ITERATION(10, task_handle, task_deletion_mutex);
    }
}
