#ifndef MAX30102_TYPES_H
#define MAX30102_TYPES_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/i2c_master.h"

typedef enum
{
    FIFO_ROLLOVER_ENABLE,
    FIFO_ROLLOVER_DISABLE
} fifo_rollover_t;

/*Number of samples to average*/
typedef enum
{
    FIFO_AVG_SAMPLE_1 = 0,
    FIFO_AVG_SAMPLE_2,
    FIFO_AVG_SAMPLE_4,
    FIFO_AVG_SAMPLE_8,
    FIFO_AVG_SAMPLE_16,
    FIFO_AVG_SAMPLE_32,
} sample_average_t;

typedef enum
{
    SPO2_RANGE_2048 = 0,
    SPO2_RANGE_4096,
    SPO2_RANGE_8192,
    SPO2_RANGE_16384,
} spo2_adc_range_t;

typedef enum
{
    SPO2_SAMPLE_RATE_50_HZ = 0,
    SPO2_SAMPLE_RATE_100_HZ,
    SPO2_SAMPLE_RATE_200_HZ,
    SPO2_SAMPLE_RATE_400_HZ,
    SPO2_SAMPLE_RATE_800_HZ,
    SPO2_SAMPLE_RATE_1000_HZ,
    SPO2_SAMPLE_RATE_1600_HZ,
    SPO2_SAMPLE_RATE_3200_HZ,
} spo2_sample_rate;

typedef enum
{
    LED_ADC_RESOLUTION_15_BIT = 0,
    LED_ADC_RESOLUTION_16_BIT,
    LED_ADC_RESOLUTION_17_BIT,
    LED_ADC_RESOLUTION_18_BIT,
} led_adc_resolution_t; // Sets resolution for ir and red LED's

typedef enum
{
    SHUTDOWN_ENABLE_MASK,
    SHUTDOWN_DISABLE_MASK,
} shutdown_status_t;

typedef enum
{
    RESET_DISABLE,
    RESET_ENABLE,
} reset_enable_t;

typedef enum
{
    LED_HR_ONLY_MODE,
    LED_SPO2_ONLY_MODE,
    LED_MULTI_LED_MODE,
} led_mode_t;

typedef struct
{
    bool almost_full_intr;
    bool ppg_ready_intr;
    bool alc_overflow_intr;
    bool die_temp_ready_intr;
} intr_enable_t;

typedef struct
{
    reset_enable_t enable_reset;            // Enable resetting cofig registers before writing settings again
    intr_enable_t intr_enable_flags;        // Interrupts to enable 
    sample_average_t sample_average;        // Num sumples to average
    fifo_rollover_t fifo_rollover;          // Enable/disable fifo
    uint8_t fifo_unread_samples_intr;             // Number of full samples to trigger interrupt 
    led_adc_resolution_t adc_resolution;    // The sample resolution
    spo2_sample_rate spo2_sample_rate;      // Samples taken per second
    spo2_adc_range_t spo2_range;            // Range of numbers returned
    led_mode_t led_mode;                    // Heart rate only, Spo2 or both
    uint8_t red_led_amplitude;              // Red LED power draw
    uint8_t ir_led_amplitude;               // Ir LED power draw
} max30102_cfg_t;

typedef struct
{
    SemaphoreHandle_t fifo_semaphore;
    i2c_master_dev_handle_t sensor_dev_handle;
} max30102_handle_internal_t;

#endif // MAX30102_TYPES_H