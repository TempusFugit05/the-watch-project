#include "widget.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <ctime>
#include <random>

#include "ds3231/ds3231.h"

// #define CHECK_TASK_SIZES

widget::widget(hagl_backend_t* display, SemaphoreHandle_t display_mutex, hagl_window_t clip, const char* widget_name) :
display_handle(display), display_mutex(display_mutex), clip(clip)
{
    task_deletion_mutex = xSemaphoreCreateBinary();
    snprintf(WIDGET_NAME, MAX_WIDGET_NAME_LENGTH, widget_name);
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