#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"

#include "input_event_types.h"
#include "config.h"
#include "isrs.h"

void IRAM_ATTR gpio_button_isr_handler(void* timer)
{
    gptimer_handle_t gptimer = *static_cast<gptimer_handle_t*>(timer);

    gptimer_stop(gptimer); // Stop timer before setting counting start

    if (gpio_get_level(GPIO_BUTTON_PIN))
    {
        gptimer_set_raw_count(gptimer, BUTTON_DEBOUNCE_COOLDOWN); // Reset timer count
    }
    else
    {
        gptimer_set_raw_count(gptimer, BUTTON_LONG_PRESS_COOLDOWN); // Start long press cooldown if button is pressed
    }

    gptimer_start(gptimer); // Start timer
}

bool IRAM_ATTR button_timer_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* queue_ptr)
{
    static bool disable_short_press = false;
    input_event_t event;
    BaseType_t should_yield = pdFALSE;
    QueueHandle_t queue = *static_cast<QueueHandle_t*>(queue_ptr);

    if (gpio_get_level(GPIO_BUTTON_PIN)) // When button is depressed (Short press behavior)
    {
        if (!disable_short_press)
        {
            event = SHORT_PRESS_EVENT;
            xQueueSendFromISR(queue, &event, &should_yield); // Pose event to queue
        }
        else // Re-enable short press behavior
        {
            disable_short_press = false;
        }
    }

    else // If button is pressed (Long press behavior)
    {
        disable_short_press = true; // Disable short press once to prevent both press types occuring at once
        event = LONG_PRESS_EVENT;
        xQueueSendFromISR(queue, &event, &should_yield); // Pose event to queue
    }

    if (should_yield != pdFALSE)
    {
       portYIELD_FROM_ISR();
    }

    return false;
}

void IRAM_ATTR gpio_encoder_isr_handler(void* queue_ptr)
{
/*
This function reads the states of the encoder pins and compares their states
Since the contacts connected to the pins are offset by 90 degrees inside the encoder,
one of them will always change before the other when rotating clockwise and vise-versa
*/
    bool CLK_state = gpio_get_level(ENCODER_CLK_PIN);
    static bool reference_CLK_state = gpio_get_level(ENCODER_CLK_PIN);

    input_event_t event;
    BaseType_t should_yield = pdFALSE;
    QueueHandle_t queue = *static_cast<QueueHandle_t*>(queue_ptr);

    if (reference_CLK_state != CLK_state)
    {
        reference_CLK_state = CLK_state;

        if (CLK_state != gpio_get_level(ENCODER_DT_PIN))
        {
            event = SCROLL_RIGHT_EVENT;
            xQueueSendFromISR(queue, &event, &should_yield);
        }
        else
        {
            event = SCROLL_LEFT_EVENT;
            xQueueSendFromISR(queue, &event, &should_yield);
        }

    }

    if (should_yield != pdFALSE)
    {
        portYIELD_FROM_ISR();
    }
}
