#ifndef WIDGET_H
#define WIDGET_H

#include <ctime>

#include "hagl_hal.h"
#include "hagl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds3231/ds3231.h"
#include "input_event_types.h"
#include "widget_utils.h"

#define MAX_WIDGET_NAME_LENGTH 64

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

class widget
{    
protected:
    hagl_backend_t* display_handle;
    SemaphoreHandle_t display_mutex;
    TaskHandle_t task_handle;
    input_event_t input_event = INIT_EVENT;
    hagl_window_t clip;

    char WIDGET_NAME[MAX_WIDGET_NAME_LENGTH];

    SemaphoreHandle_t task_deletion_mutex;

    bool requires_inputs = false;
    bool set_input_requirement(bool requirement_state);

public:
    widget(hagl_backend_t* display_handle, SemaphoreHandle_t  display_mutex, hagl_window_t clip, const char* widget_name);
    input_event_t set_input_event(input_event_t event);
    bool get_input_requirements();
    
    virtual ~widget() = 0;
    virtual void run_widget() = 0;
};

void inline call_run_widget(void* widget_obj)
{
    /*FreeRTOS expects a pointer to a function. C++ member functions, however are not pointers to a function
    and there is no way to convert them to that. Therefore, this function calls the member function itself when freertos calls it.*/
    static_cast<widget*>(widget_obj)->run_widget();
}

#endif // WIDGET