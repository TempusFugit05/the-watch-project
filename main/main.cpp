#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "hagl_hal.h"
#include "hagl.h"
#include "string.h"

#include "widgets.h"
#include "ds3231/ds3231.h"
#include "isrs.h"
#include "setup.h"

hagl_backend_t* display; // Main display 
SemaphoreHandle_t display_mutex = xSemaphoreCreateMutex(); // Mutex to ensure two tasks aren't writing at the same time

ds3231_handle_t ds3231_dev_handle;

QueueHandle_t input_event_queue = xQueueCreate(10, sizeof(input_event_t)); // Queue for input events (used in input isrs and input event task)

#include "widget_manager.h"


extern "C" void app_main(void)
{
    // i2c setup
    static i2c_master_bus_handle_t i2c_master_handle;
    setup_i2c_master(&i2c_master_handle);
    
    // RTC setup
    ds3231_dev_handle = ds3231_init(&i2c_master_handle);
    ESP_ERROR_CHECK(ds3231_set_datetime_at_compile(&ds3231_dev_handle, &i2c_master_handle, false));

    // Display setup
    display = hagl_init();
    hagl_clear(display);

    setup_gpio(); // Set up the gpio pins for inputs
    static gptimer_handle_t button_timer = setup_gptimer(&input_event_queue); // Create timer for software debounce
    setup_isrs(&input_event_queue, &button_timer); // Set up input interrupts

    main_screen_state_machine(INIT_EVENT);
    
    // Task to handle the various input interrupt signals
    TaskHandle_t input_events_handler_task_handle;
    xTaskCreatePinnedToCore(input_events_handler_task, "input_events_handler_task", 2048, NULL, 5, &input_events_handler_task_handle, 0);
}