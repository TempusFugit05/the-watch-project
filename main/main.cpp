#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "driver/gpio.h"    
#include "driver/gptimer.h"
#include "driver/i2c_master.h"

#include "hagl_hal.h"
#include "hagl.h"
#include "string.h"

#include "widgets/widgets.h"
#include "ds3231/ds3231.h"
#include "isrs.h"

#define I2C_SCL_PIN         GPIO_NUM_22
#define I2C_SDA_PIN         GPIO_NUM_21
#define I2C_FREQ_HZ         1000000

// #define CHECK_TASK_SIZES

hagl_backend_t* display; // Main display 
SemaphoreHandle_t display_mutex = xSemaphoreCreateMutex(); // Mutex to ensure two tasks aren't writing at the same time

ds3231_handle_t ds3231_dev_handle;

gptimer_handle_t button_timer; // Timer for button software debounce

i2c_master_bus_handle_t i2c_master_handle;

QueueHandle_t input_event_queue = xQueueCreate(10, sizeof(input_event_t)); // Queue for input events (used in input isrs and input event task)

#include "input_manager.h"
#include "widget_manager.h"

i2c_master_bus_handle_t setup_i2c_master()
{
    // Set up the i2c master bus
    ESP_LOGI("Setup", "Setting up i2c bus");
    i2c_master_bus_config_t i2c_master_conf =
    {
        .i2c_port = 1,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags{.enable_internal_pullup = true}
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_conf, &i2c_master_handle));
    return i2c_master_handle;
}

extern "C" void app_main(void)
{
    // i2c setup
    setup_i2c_master();

    // Display setup
    display = hagl_init();
    hagl_clear(display);

    setup_gpio(); // Set up the gpio pins for inputs
    button_timer = setup_gptimer(&input_event_queue); // Create timer for software debounce
    setup_isrs(&input_event_queue, &button_timer); // Set up input interrupts

    ds3231_dev_handle = ds3231_init(&i2c_master_handle);
    ESP_ERROR_CHECK(ds3231_set_datetime_at_compile(&ds3231_dev_handle, &i2c_master_handle, false));

    main_screen_state_machine(INIT_EVENT);
    
    // Task to handle the various input interrupt signals
    TaskHandle_t input_events_handler_task_handle;
    xTaskCreatePinnedToCore(input_events_handler_task, "input_events_handler_task", 2048, NULL, 5, &input_events_handler_task_handle, 0);
}