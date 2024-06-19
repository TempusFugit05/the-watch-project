#ifndef CONFIG_H
#define CONFIG_H

#define I2C_SCL_PIN         GPIO_NUM_22
#define I2C_SDA_PIN         GPIO_NUM_21

#define BUTTON_DEBOUNCE_COOLDOWN    10000 // Debounce time for the button (µs)
#define BUTTON_LONG_PRESS_COOLDOWN  500000 // Delay time to register a long-press (µs)

#define GPIO_BUTTON_PIN     GPIO_NUM_34
#define ENCODER_CLK_PIN     GPIO_NUM_39
#define ENCODER_DT_PIN      GPIO_NUM_36

#define LED_PIN             GPIO_NUM_2
#define LED_PIN2            GPIO_NUM_25

#endif // CONFIG_H