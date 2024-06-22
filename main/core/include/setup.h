#ifndef SETUP_H
#define SETUP_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "driver/gpio.h"    
#include "driver/gptimer.h"
#include "driver/i2c_master.h"

#include "isrs.h"
#include "config.h"

gptimer_handle_t setup_gptimer(QueueHandle_t* queue_ptr);
void setup_gpio();
void setup_isrs(QueueHandle_t* event_queue, gptimer_handle_t* timer);
void setup_i2c_master(i2c_master_bus_handle_t* bus_handle);
void setup_display();
#endif // SETUP_H