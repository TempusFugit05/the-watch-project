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

#include "ds3231.h"
#include "input_event_types.h"

#define LED_PIN             GPIO_NUM_2
#define LED_PIN2            GPIO_NUM_25

#define GPIO_BUTTON_PIN     GPIO_NUM_34
#define ENCODER_CLK_PIN     GPIO_NUM_39
#define ENCODER_DT_PIN      GPIO_NUM_36

#define I2C_SCL_PIN         GPIO_NUM_22
#define I2C_SDA_PIN         GPIO_NUM_21
#define I2C_FREQ_HZ         1000000

#define BUTTON_DEBOUNCE_COOLDOWN    10000
#define BUTTON_LONG_PRESS_COOLDOWN  500000

// #define CHECK_TASK_SIZES

hagl_backend_t* display; // Main display 
SemaphoreHandle_t display_mutex = xSemaphoreCreateMutex(); // Mutex to ensure two tasks aren't writing at the same time

ds3231_handle_t ds3231_dev_handle;

gptimer_handle_t button_timer; // Timer for button software debounce

i2c_master_bus_handle_t i2c_master_handle;

typedef enum : uint8_t{
    SCREEN_FACES_PADDING_LOWER,
    SCREEN_CLOCK_FACE,
    SCREEN_HEART_RATE_FACE,
    SCREEN_FACES_PADDING_UPPER,
}main_screen_faces_t;

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
            event = SCROLL_DOWN_EVENT;
            xQueueSendFromISR(input_event_queue, &event, &should_yield);
        }
        else
        {
            event = SCROLL_UP_EVENT;
            xQueueSendFromISR(input_event_queue, &event, &should_yield);
        }        

    }

    if (should_yield != pdFALSE)
    {
        portYIELD_FROM_ISR();
    }
}

bool input_event_redirector(widget* active_widget, input_event_t event)
{
    /*This function is responsible for redirecting the input events to the relevant widgets*/
    if (active_widget != NULL)
    {
        if (active_widget->get_input_requirements())
        {
            active_widget->set_input_event(event);
            return true; // True if input is redirected
        }
    } // Check if the active widget requires inputs

    return false; // False if input is not redirected
}

void main_screen_state_machine(input_event_t event)
{
    /*This function handles the switching of ui elements upon input events*/

    /*Currently running ui elements*/
    static main_screen_faces_t current_face = SCREEN_FACES_PADDING_LOWER;
    static widget *widget_instance = NULL;    
    static info_bar_widget *info_bar = NULL; //new info_bar_widget(display, display_mutex, &ds3231_dev_handle);

    if (event == INIT_EVENT)
    {
        widget_instance = new snake_game_widget(display, display_mutex);
        // widget_instance = new clock_widget(display, display_mutex, &ds3231_dev_handle);
        current_face = SCREEN_CLOCK_FACE;
    } // Initialize the clock widget upon startup



    if (!input_event_redirector(widget_instance, event)) // Check if input needs redirecting to widget
    {
        switch (event)
        {
        case SCROLL_UP_EVENT:
            current_face = static_cast<main_screen_faces_t>(current_face + static_cast <main_screen_faces_t>(1));
            if (current_face >= SCREEN_FACES_PADDING_UPPER)
            {
                current_face = static_cast<main_screen_faces_t>(SCREEN_FACES_PADDING_LOWER + 1);
            } // Switch to the next widget upon scrolling the encoder CCW
            break;
        
        case SCROLL_DOWN_EVENT:
            current_face = static_cast<main_screen_faces_t>(current_face - static_cast <main_screen_faces_t>(1));
            if (current_face <= SCREEN_FACES_PADDING_LOWER)
            {
                current_face = static_cast<main_screen_faces_t>(SCREEN_FACES_PADDING_UPPER - 1);
            } // Switch to the previous widget upon scrolling the encoder CC
            break;

        default:
            break;
        }
        
        if (event == SCROLL_UP_EVENT || event == SCROLL_DOWN_EVENT)
        {
            switch (current_face)
            {
                case SCREEN_CLOCK_FACE:
                    {
                        if (widget_instance != NULL)
                        {
                            delete widget_instance;
                        }
                        widget_instance = new clock_widget(display, display_mutex, &ds3231_dev_handle);
                        delete info_bar;
                        break;
                    } // Create clock widget and delete info bar widget if it exists

                case SCREEN_HEART_RATE_FACE:
                {
                    if (widget_instance != NULL)
                    {
                        delete widget_instance;
                    }
                    widget_instance = new test_widget(display, display_mutex);
                    info_bar = new info_bar_widget(display, display_mutex, &ds3231_dev_handle);
                    break;
                }
                default:
                    break;
            } // Create widget based on the current screen face
        }
    }
}

void input_events_handler_task(void* arg)
{
    /*This tasks is responisble for handling the input events from buttons and encoders.
    It gets the info from a queue to which the input ISR's handlers send a message when they are activated*/
    
    input_event_t event; // Event type
    char event_id[32];

    /*Delay amount to prevent switching faces too fast*/
    static const unsigned long face_switch_delay_ms = pdMS_TO_TICKS(50); 
    static unsigned long time_since_last_update = 0;

    while(1){
        if(xQueueReceive(input_event_queue, &event, portMAX_DELAY)) // Wait for an event in the queue
        {
            unsigned long current_time = xTaskGetTickCount();
            if (current_time - time_since_last_update >= face_switch_delay_ms)
            {
                switch (event)
                {
                    case SHORT_PRESS_EVENT:
                        static bool led_state = false;
                        led_state = !led_state;
                        gpio_set_level(LED_PIN, led_state);
                        sprintf(event_id, "SHORT_PRESS_EVENT");
                        break;
                    
                    case LONG_PRESS_EVENT:
                        static bool led2_state = false;
                        led2_state = !led2_state;
                        gpio_set_level(LED_PIN2, led2_state);
                        sprintf(event_id, "LONG_PRESS_EVENT");
                        break;

                    case SCROLL_UP_EVENT:
                        sprintf(event_id, "SCROLL_UP_EVENT");
                        break;

                    case SCROLL_DOWN_EVENT:
                        sprintf(event_id, "SCROLL_DOWN_EVENT");
                        break;
                        
                    default:
                        break;
                }
                main_screen_state_machine(event);
                time_since_last_update = current_time; // Update the time at switching
            }
        // ESP_LOGI("input_event", "%s", event_id);
        }

        #ifdef CHECK_TASK_SIZES
            ESP_LOGI(TAG, "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        
    }
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

void setup_isrs()
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
    button_timer = setup_gptimer(); // Create timer for software debounce
    setup_isrs(); // Set up input interrupts

    ds3231_dev_handle = ds3231_init(&i2c_master_handle);
    ESP_ERROR_CHECK(ds3231_set_datetime_at_compile(&ds3231_dev_handle, &i2c_master_handle, false));

    main_screen_state_machine(INIT_EVENT);
    
    // Task to handle the various input interrupt signals
    TaskHandle_t input_events_handler_task_handle;
    xTaskCreatePinnedToCore(input_events_handler_task, "input_events_handler_task", 2048, NULL, 5, &input_events_handler_task_handle, 0);
}