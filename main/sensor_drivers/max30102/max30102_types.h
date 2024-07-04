#ifndef MAX30102_TYPES_H
#define MAX30102_TYPES_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/i2c_master.h"

#include "max30102_configs.h"

typedef struct
{
    bool almost_full_intr;
    bool ppg_ready_intr;
    bool alc_overflow_intr;
    bool power_ready_intr;
    bool die_temp_ready_intr;
} intr_enable_t;

typedef struct
{
    intr_enable_t intr_enable_flags; // Interrupts to enable 
    sample_average_t sample_average; // Num sumples to average
    fifo_rollover_t fifo_rollover; // Enable/disable fifo
    uint8_t fifo_fill_num_intr; // Number of full samples to trigger interrupt 
    reset_enable_t enable_reset;
    led_mode_t led_mode;
} max30102_cfg_t;

typedef struct
{
    SemaphoreHandle_t fifo_semaphore;
    i2c_master_dev_handle_t sensor_dev_handle;
} max30102_handle_internal_t;

typedef struct
{
    uint8_t intr_mask_1;
    uint8_t intr_mask_2;
    max30102_cfg_t* user_config;
} config_internal_t;

#endif // MAX30102_TYPES_H