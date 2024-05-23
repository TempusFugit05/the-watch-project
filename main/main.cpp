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
#include "fonts/font9x18B-ISO8859-1.h"
#include "fonts/font10x20-KOI8-R.h"
#include "font6x9.h"

#include "ds3231.h"
#include "string.h"

#define LED_PIN             GPIO_NUM_2
#define GPIO_VIBRATOR_PIN   GPIO_NUM_25

#define GPIO_BUTTON_PIN     GPIO_NUM_34
#define ENCODER_CLK_PIN     GPIO_NUM_39
#define ENCODER_DT_PIN      GPIO_NUM_36

#define I2C_SCL_PIN         GPIO_NUM_22
#define I2C_SDA_PIN         GPIO_NUM_21
#define I2C_FREQ_HZ         1000000

#define OLED_REFRESH_RATE   60

#define BUTTON_DEBOUNCE_COOLDOWN    10000
#define BUTTON_LONG_PRESS_COOLDOWN  500000

// #define SET_COMPILE_TIME_FOR_RTC    false

#define SCEEN_SIZE_X 240
#define SCEEN_SIZE_Y 320

hagl_backend_t display; // Main display 

gptimer_handle_t button_timer; // Timer for button software debounce

ds3231_handle_t ds3231_dev_handle; // Rtc

tm rtc_time; // Time from the rtc module

typedef enum : uint8_t{
    FOO,
    SHORT_PRESS_EVENT,
    LONG_PRESS_EVENT,
    SCROLL_UP_EVENT,
    SCROLL_DOWN_EVENT,
}input_event_t; // Input event flags for the input events queue

QueueHandle_t input_event_queue = xQueueCreate(10, sizeof(input_event_t)); // Queue for input events (used in input isrs and input event task)

static void IRAM_ATTR gpio_button_isr_handler(void* arg)
{
    gptimer_stop(button_timer); // Stop timer before setting counting start

    if (gpio_get_level(GPIO_BUTTON_PIN))
    {
        gptimer_set_raw_count(button_timer, BUTTON_DEBOUNCE_COOLDOWN); // Reset timer count
    }
    else
    {
        gptimer_set_raw_count(button_timer, BUTTON_LONG_PRESS_COOLDOWN); // Start long press cooldown if button is pressed
    }

    gptimer_start(button_timer); // Start timer
}

static bool IRAM_ATTR button_timer_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* arg)
{
    static bool disable_short_press = false;
    input_event_t event;
    BaseType_t should_yield = pdFALSE;

    if (gpio_get_level(GPIO_BUTTON_PIN)) // When button is depressed (Short press behavior)
    {
        if (!disable_short_press)
        {
            event = SHORT_PRESS_EVENT;
            xQueueSendFromISR(input_event_queue, &event, &should_yield); // Pose event to queue
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
        xQueueSendFromISR(input_event_queue, &event, &should_yield); // Pose event to queue
    }

    if (should_yield != pdFALSE)
    {
       portYIELD_FROM_ISR();
    }

    return false;
}

static void IRAM_ATTR gpio_encoder_isr_handler(void* arg)
{
/*
This function reads the states of the encoder pins and compares their states
Since the contacts connected to the pins are offset by 90 degrees inside the encoder,
one of them will always change before the other when rotating clockwise and vise-versa
*/
    bool CLK_state = gpio_get_level(ENCODER_CLK_PIN);
    static bool reference_CLK_state = false;
    
    input_event_t event;
    BaseType_t should_yield = pdFALSE;

    if (reference_CLK_state != CLK_state)
    {
        reference_CLK_state = CLK_state;

        if (CLK_state != gpio_get_level(ENCODER_DT_PIN))
        {
            event = SCROLL_UP_EVENT;
            xQueueSendFromISR(input_event_queue, &event, &should_yield);
        }
        else
        {
            event = SCROLL_DOWN_EVENT;
            xQueueSendFromISR(input_event_queue, &event, &should_yield);
        }        

    }

    if (should_yield != pdFALSE)
    {
        portYIELD_FROM_ISR();
    }
}

void input_events_handler(void* arg)
{
    /*This tasks is responisble for handling the input events from buttons and encoders.
    It gets the info from a queue to which the input ISR's handlers send a message when they are activated*/
    
    input_event_t event; // Event type
    char* event_id = "";
    while(1){
        if(xQueueReceive(input_event_queue, &event, portMAX_DELAY)) // Wait for an event in the queue
        {
            switch (event)
            {
                case SHORT_PRESS_EVENT:
                    static bool led_state = false;
                    led_state = !led_state;
                    gpio_set_level(LED_PIN, led_state);
                    event_id = "SHORT_PRESS_EVENT";
                    break;
                
                case LONG_PRESS_EVENT:
                    static bool vibrator_state = false;
                    vibrator_state = !vibrator_state;
                    gpio_set_level(GPIO_VIBRATOR_PIN, vibrator_state);
                    event_id = "LONG_PRESS_EVENT";
                    break;

                case SCROLL_UP_EVENT:
                    event_id = "SCROLL_UP_EVENT";
                    break;

                case SCROLL_DOWN_EVENT:
                    event_id = "SCROLL_DOWN_EVENT";
                    break;
                default:
                    break;
            }
        ESP_LOGI("input_event", "%s", event_id);
        ESP_LOGI("task_size", "%i", uxTaskGetStackHighWaterMark(NULL));
        }
    }
}

void get_time()
{

}

void write_time(void* arg)
{
    const TickType_t task_delay_ms = 1000 / portTICK_PERIOD_MS;

    hagl_color_t color = hagl_color(&display, 255, 0, 0); // Placeholder color (Note: the r and b channels are inverted on my display)
    char time_str [32]; // String buffer
    wchar_t formatted_str [32]; // String buffer compatable with the display library
    while (1)
    {
        ds3231_get_datetime(&ds3231_dev_handle, &rtc_time); // Get time
        snprintf(time_str, 64, "%02i:%02i:%02i", 
        rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);
        mbstowcs(formatted_str, time_str, 32);
        hagl_put_text(&display, formatted_str, DISPLAY_WIDTH/2 - strlen(time_str)*4, (DISPLAY_HEIGHT/2 - strlen(time_str)*9), color, font10x20_KOI8_R); // Display string
        
        snprintf(time_str, 64, "%02i,%02i,%04i", 
        rtc_time.tm_mday, rtc_time.tm_mon, rtc_time.tm_year);
        mbstowcs(formatted_str, time_str, 32);
        hagl_put_text(&display, formatted_str, DISPLAY_WIDTH/2 - strlen(time_str)*4, (DISPLAY_HEIGHT/2 - strlen(time_str)*9) + 40, color, font10x20_KOI8_R); // Display string

        vTaskDelay(task_delay_ms); // Delay for 1 second
    }
    ESP_LOGI("task_size", "%i", uxTaskGetStackHighWaterMark(NULL));
}
gptimer_handle_t setup_gptimer()
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
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&button_timer_conf, &gptimer)); // Create timer object

    gptimer_alarm_config_t button_timer_alarm_conf = 
    {
        .alarm_count = 0,
        .reload_count = 0,
    };
    button_timer_alarm_conf.flags.auto_reload_on_alarm = false;

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &button_timer_alarm_conf));
    gptimer_event_callbacks_t cbs = 
    {
        .on_alarm = button_timer_isr_handler,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

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
        .pin_bit_mask = (1ULL << LED_PIN | 1ULL << GPIO_VIBRATOR_PIN), // Bitmask to select the GPIO pins
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

void setup_isr()
{
    ESP_LOGI("Setup", "Setting up interrupts");

    /*Set up the isrs of the input devices*/
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM)); // Set up interrupt

    // Button isr
    ESP_ERROR_CHECK(gpio_set_intr_type(GPIO_BUTTON_PIN, GPIO_INTR_ANYEDGE)); // Interrupt will be called on button release since 
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON_PIN, gpio_button_isr_handler, NULL)); // Link to interrupt handler & pass pointer to the timer object when called

    // Encoder (DT pin) isr
    ESP_ERROR_CHECK(gpio_set_intr_type(ENCODER_DT_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_DT_PIN, gpio_encoder_isr_handler, NULL));

    // Encoder (CLK pin) isr
    ESP_ERROR_CHECK(gpio_set_intr_type(ENCODER_CLK_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_CLK_PIN, gpio_encoder_isr_handler, NULL));    
}

i2c_master_bus_handle_t setup_i2c_master()
{
    // Set up the i2c master bus
    i2c_master_bus_config_t i2c_master_conf =
    {
        .i2c_port = 1,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
    };
    i2c_master_conf.flags.enable_internal_pullup = true;
    i2c_master_bus_handle_t i2c_master_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_conf, &i2c_master_handle));
    return i2c_master_handle;
}

extern "C" void app_main(void)
{
    // i2c setup
    i2c_master_bus_handle_t i2c_master_handle = setup_i2c_master();
    
    // Rtc setup
    ds3231_dev_handle = ds3231_init(&i2c_master_handle, DS3231_TIME_FORMAT_24_HOURS); // rtc setup

    // Set Rtc time to compilation time
    #ifdef SET_COMPILE_TIME_FOR_RTC
        ds3231_set_datetime_at_compile(&ds3231_dev_handle);
    #endif

    // Display setup
    display = *hagl_init();
    hagl_clear(&display);
    
    setup_gpio(); // Set up the gpio pins for inputs
    button_timer = setup_gptimer(); // Create timer for software debounce
    setup_isr(); // Set up input interrupts

    // Task to handle the various input interrupt signals
    TaskHandle_t input_events_handler_task;
    xTaskCreatePinnedToCore(input_events_handler, "write_time_task", 4096, NULL, 4, &input_events_handler_task, 0);

    // Time-keeping task
    TaskHandle_t write_time_task;
    xTaskCreatePinnedToCore(write_time, "write_time_task", 4096, NULL, 4, &write_time_task, 0);
    }