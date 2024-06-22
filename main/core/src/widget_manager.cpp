#include "widget_manager.h"
#include "lvgl.h"
#include "input_event_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

// #define CHECK_TASK_SIZES

static int16_t encoder_difference = 0;
static bool is_button_pressed = 0;

void encoder_event_callback(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    if (encoder_difference != 0)
    {
        data->enc_diff = encoder_difference;
        encoder_difference = 0;
    }
    data->state = is_button_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    is_button_pressed = false;
}

void input_events_handler_task(void* queue_ptr)
{
    /*This tasks is responisble for handling the input events from buttons and encoders.
    It gets the info from a queue to which the input ISR's handlers send a message when they are activated*/
    QueueHandle_t event_queue = *(QueueHandle_t*) queue_ptr;
    input_event_t event; // Event type

    while(1){
        if(xQueueReceive(event_queue, &event, portMAX_DELAY)) // Wait for an event in the queue
        {
            switch (event)
            {
            case SCROLL_LEFT_EVENT:
                encoder_difference--;
                break;

            case SCROLL_RIGHT_EVENT:
                encoder_difference++;
                break;

            case SHORT_PRESS_EVENT:
                is_button_pressed = true;
                break;

            default:
                break;
            }
        }

        #ifdef CHECK_TASK_SIZES
            ESP_LOGI("", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif

    }
}
