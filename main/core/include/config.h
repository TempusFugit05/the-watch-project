#ifndef CONFIG_H
#define CONFIG_H
#include "driver/gpio.h"
#include "driver/ledc.h"

#define I2C_SCL_PIN         GPIO_NUM_13
#define I2C_SDA_PIN         GPIO_NUM_14

/*Display config*/
#define DISPLAY_LED_PIN     GPIO_NUM_37
#define DISPLAY_CS_PIN      GPIO_NUM_36
#define DISPLAY_DC_PIN      GPIO_NUM_35
#define DISPLAY_MOSI_PIN    GPIO_NUM_47
#define DISPLAY_SCLK_PIN    GPIO_NUM_21
#define DISPLAY_RST_PIN     GPIO_NUM_48
#define DISPLAY_SIZE_X      240
#define DISPLAY_SIZE_Y      240

/*Display LED config*/
#define DISPLAY_LEDC_CHANNEL        LEDC_CHANNEL_0
#define DIAPLAY_LEDC_RESOLUTION     LEDC_TIMER_10_BIT
#define DISPLAY_FADE_IN_TIME_MS     1000

/*Encode input config*/
#define BUTTON_DEBOUNCE_COOLDOWN    10000 // Debounce time for the button (µs)
#define BUTTON_LONG_PRESS_COOLDOWN  500000 // Delay time to register a long-press (µs)
#define GPIO_BUTTON_PIN     GPIO_NUM_38
#define ENCODER_CLK_PIN     GPIO_NUM_40
#define ENCODER_DT_PIN      GPIO_NUM_39


#endif // CONFIG_H
