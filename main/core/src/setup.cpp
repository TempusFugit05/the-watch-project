#include "esp_system.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"

#include "isrs.h"
#include "config.h"
#include "setup.h"

#include "lvgl.h"
#include "demos/stress/lv_demo_stress.h"
#include "esp_lvgl_port.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "demos/benchmark/lv_demo_benchmark.h"

#include "widget_manager.h"

const char TAG[] = "Setup";

gptimer_handle_t setup_gptimer(QueueHandle_t* queue_ptr)
{
    /*Timer interrupt setup (for button)*/

    ESP_LOGI(TAG, "Setting up timer");

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
    ESP_LOGI(TAG, "Setting up GPIO");

    // LED setup
    // gpio_config_t led_io_conf =
    // {
    //     .pin_bit_mask = (1ULL << DISPLAY_LED_PIN), // Bitmask to select the GPIO pins
    //     .mode = GPIO_MODE_OUTPUT, // Set as output mode
    //     .pull_up_en = GPIO_PULLUP_DISABLE, // Disable pull-up resistor
    //     .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down resistor
    //     .intr_type = GPIO_INTR_DISABLE, // Disable interrupt
    // };
    // gpio_config(&led_io_conf); // Apply configuration
    // gpio_set_level(DISPLAY_LED_PIN, 1);

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
    ESP_LOGI(TAG, "Setting up interrupts");

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

void setup_i2c_master(i2c_master_bus_handle_t* bus_handle)
{
    // Set up the i2c master bus
    ESP_LOGI(TAG, "Setting up i2c bus");
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
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_conf, bus_handle));
}

void setup_display_spi_bus()
{
    ESP_LOGI(TAG, "Setting up SPI bus");
    spi_bus_config_t spi_config =
    {
        .mosi_io_num = DISPLAY_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = DISPLAY_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &spi_config, SPI_DMA_CH_AUTO));
}

void setup_lcd_ledc()
{
    ledc_timer_config_t ledc_timer_conf =
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = DIAPLAY_LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER_3,
        .freq_hz = 1000,
        .deconfigure = false,
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_conf));

    ledc_channel_config_t ledc_conf =
    {
        .gpio_num = DISPLAY_LED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = DISPLAY_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE, // Might want to change later to indicate end of fading
        .timer_sel = LEDC_TIMER_3,
        .duty = 0, // Set to min birghtness
        .hpoint = 0, // Not really needed for simple fading
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_conf));

    ESP_ERROR_CHECK(ledc_fade_func_install(ESP_INTR_FLAG_LEVEL3));

    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, DISPLAY_LEDC_CHANNEL,
    2<<(DIAPLAY_LEDC_RESOLUTION-1), DISPLAY_FADE_IN_TIME_MS, LEDC_FADE_NO_WAIT));
}

void test_callback(lv_event_t* event)
{
    static int counter = 0;
    counter ++;
    ESP_LOGI("BUTTON", "Clicks: %i", counter);
}

void setup_display()
{
    ESP_LOGI(TAG, "Setting up display");

    setup_display_spi_bus();

    esp_lcd_panel_io_spi_config_t display_config =
    {
        .cs_gpio_num = DISPLAY_CS_PIN,
        .dc_gpio_num = DISPLAY_DC_PIN,
        .spi_mode = 0,
        .pclk_hz = 40000000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &display_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_dev_config =
    {
        .reset_gpio_num = DISPLAY_RST_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    esp_lcd_panel_handle_t display_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_dev_config, &display_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(display_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(display_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(display_handle, true));

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    lvgl_port_display_cfg_t display_port_config =
    {
        .io_handle = io_handle,
        .panel_handle = display_handle,
        .buffer_size = DISPLAY_SIZE_X*DISPLAY_SIZE_Y,
        .double_buffer = true,
        .hres = DISPLAY_SIZE_X,
        .vres = DISPLAY_SIZE_Y,
        .monochrome = false,
        .rotation = {.swap_xy = false, .mirror_x = false, .mirror_y = false},
        .flags = {.buff_dma = true},
    };


    lv_init(); // Initialize lvgl library

    lv_disp_t* display = lvgl_port_add_disp(&display_port_config);

    // Create test button
    lv_obj_t* button = lv_btn_create(lv_scr_act());
    lv_obj_center(button);
    lv_obj_add_event_cb(button, test_callback, LV_EVENT_PRESSED, NULL);

    // Create button text
    lv_obj_t* text = lv_label_create(button);
    lv_label_set_text(text, "CLICK ME!");
    lv_obj_center(text);

    // Create encoder driver to handle inputs
    static lv_indev_drv_t encoder_driver;
    lv_indev_drv_init(&encoder_driver);
    encoder_driver.type = LV_INDEV_TYPE_ENCODER;
    encoder_driver.read_cb = encoder_event_callback;
    lv_indev_t* encoder = lv_indev_drv_register(&encoder_driver);

    // Add button to group to be able to receive inputs
    lv_group_t* button_group = lv_group_create();
    lv_group_add_obj(button_group, button);
    lv_indev_set_group(encoder, button_group);
    setup_lcd_ledc(); // Setup for the lcd led

}
