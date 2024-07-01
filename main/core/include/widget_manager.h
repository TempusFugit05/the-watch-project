#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include "lvgl.h"

#include "input_event_types.h"

void encoder_event_callback(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);
void input_events_handler_task(void* queue_ptr);

#endif // WIDGET_MANAGER_H
