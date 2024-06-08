#include "widgets.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <ctime>
#include <random>

#include "../ds3231.h"

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

widget::widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex) : display_handle(display), display_mutex(display_mutex){};
widget::~widget(){}
void widget::run_widget(){}

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
        hagl_set_clip(display_handle,0 ,20, SCREEN_SIZE_X, SCREEN_SIZE_Y); // Set drawable area
        hagl_fill_rectangle_xyxy(display_handle, 0, 20, SCREEN_SIZE_X, SCREEN_SIZE_Y, hagl_color(display_handle,0,0,0)); // Clear screen
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
            hagl_set_clip(display_handle,0 ,20, SCREEN_SIZE_X, SCREEN_SIZE_Y); // Set drawable area
            hagl_draw_circle(display_handle, SCREEN_SIZE_X/2, SCREEN_SIZE_Y/2, radius, color);
            radius += 1;

        #ifdef CHECK_TASK_SIZES
        ESP_LOGI("test_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
            xSemaphoreGive(display_mutex);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
