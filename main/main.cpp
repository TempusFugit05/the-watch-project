#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "input_event_types.h"

#include "ds3231.h"
#include "isrs.h"
#include "setup.h"

#include "new_clock.h"

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

    setup_gpio(); // Set up the gpio pins for inputs
    static gptimer_handle_t button_timer = setup_gptimer(&input_event_queue); // Create timer for software debounce
    setup_isrs(&input_event_queue, &button_timer); // Set up input interrupts

    setup_display();

    // Task to handle the various input interrupt signals
    TaskHandle_t input_events_handler_task_handle;
    xTaskCreatePinnedToCore(input_events_handler_task, "input_events_handler_task", 2048, &input_event_queue, 5, &input_events_handler_task_handle, 0);

    TaskHandle_t task;
    xTaskCreate(run_widget, "clock", 4096, &ds3231_dev_handle, 3, &task);
}