#ifndef ISRS_H
#define ISRS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "input_event_types.h"

#define BUTTON_DEBOUNCE_COOLDOWN    10000
#define BUTTON_LONG_PRESS_COOLDOWN  500000

#define GPIO_BUTTON_PIN     GPIO_NUM_34
#define ENCODER_CLK_PIN     GPIO_NUM_39
#define ENCODER_DT_PIN      GPIO_NUM_36

#define LED_PIN             GPIO_NUM_2
#define LED_PIN2            GPIO_NUM_25

static void IRAM_ATTR gpio_button_isr_handler(void* timer)
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

static bool IRAM_ATTR button_timer_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* queue_ptr)
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

static void IRAM_ATTR gpio_encoder_isr_handler(void* queue_ptr)
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
            event = SCROLL_DOWN_EVENT;
            xQueueSendFromISR(queue, &event, &should_yield);
        }
        else
        {
            event = SCROLL_UP_EVENT;
            xQueueSendFromISR(queue, &event, &should_yield);
        }        

    }

    if (should_yield != pdFALSE)
    {
        portYIELD_FROM_ISR();
    }
}

gptimer_handle_t setup_gptimer(QueueHandle_t* queue_ptr)
{
    /*Timer interrupt setup (for button)*/

    ESP_LOGI("Setup", "Setting up timer");

    gptimer_handle_t gptimer;
    gptimer_config_t button_timer_conf = 
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_DOWN,
        .resolution_hz = 1000000,
        .intr_priority = 0,
        .flags{.intr_shared=false}
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&button_timer_conf, &gptimer)); // Create timer object

    gptimer_alarm_config_t button_timer_alarm_conf = 
    {
        .alarm_count = 0,
        .reload_count = 0,
        .flags{.auto_reload_on_alarm = false}
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &button_timer_alarm_conf));
    gptimer_event_callbacks_t cbs = 
    {
        .on_alarm = button_timer_isr_handler,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, queue_ptr));

    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    return gptimer;
}

void setup_gpio()
{
    /*Configure the gpio of the esp*/
    ESP_LOGI("Setup", "Setting up GPIO");

    // LED setup
    gpio_config_t led_io_conf =
    {
        .pin_bit_mask = (1ULL << LED_PIN | 1ULL << LED_PIN2), // Bitmask to select the GPIO pins
        .mode = GPIO_MODE_OUTPUT, // Set as output mode
        .pull_up_en = GPIO_PULLUP_DISABLE, // Disable pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down resistor
        .intr_type = GPIO_INTR_DISABLE, // Disable interrupt
    };
    gpio_config(&led_io_conf); // Apply configuration
    
    // Button setup
    gpio_config_t button_io_conf =
    {
        .pin_bit_mask = (1ULL << GPIO_BUTTON_PIN), 
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&button_io_conf); 

    // Encoder pins setup
    gpio_config_t encoder_io_conf = 
    {
        .pin_bit_mask = (1ULL << ENCODER_CLK_PIN | 1ULL << ENCODER_DT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&encoder_io_conf);
}

void setup_isrs(QueueHandle_t* event_queue, gptimer_handle_t* timer)
{
    ESP_LOGI("Setup", "Setting up interrupts");

    /*Set up the isrs of the input devices*/
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM)); // Set up interrupt

    // Button isr
    ESP_ERROR_CHECK(gpio_set_intr_type(GPIO_BUTTON_PIN, GPIO_INTR_ANYEDGE)); // Interrupt will be called on button release since 
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON_PIN, gpio_button_isr_handler, timer)); // Link to interrupt handler & pass pointer to the timer object when called

    // Encoder (DT pin) isr
    ESP_ERROR_CHECK(gpio_set_intr_type(ENCODER_DT_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_DT_PIN, gpio_encoder_isr_handler, event_queue));

    // Encoder (CLK pin) isr
    ESP_ERROR_CHECK(gpio_set_intr_type(ENCODER_CLK_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_CLK_PIN, gpio_encoder_isr_handler, event_queue));    
}

#endif // ISRS_H