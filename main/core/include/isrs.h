#ifndef ISRS_H
#define ISRS_H

#include "driver/gptimer.h"

void gpio_button_isr_handler(void* timer);
bool button_timer_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* queue_ptr);
void gpio_encoder_isr_handler(void* queue_ptr);

#endif // ISRS_H