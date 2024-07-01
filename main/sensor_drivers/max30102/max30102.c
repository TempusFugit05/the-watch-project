#include "max30102.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define MAX30102_WRITE_ADDR 0xAE // I2c address for write operations
#define MAX30102_READ_ADDR  0xAF // I2c for read operations

#define MAX30102_INTR_PIN    GPIO_NUM_4 // Interrupt to signify fifo buffer almost full (active-low)

static void max30102_buff_service_task(void* i2c_bus)
{
    i2c_master_bus_handle_t* i2c_bus_handle = (i2c_master_bus_handle_t*) i2c_bus;
    while (1)
    {    
        ESP_LOGI("success", "");
        vTaskDelay(portMAX_DELAY);
    }
    
}

IRAM_ATTR void max30102_isr_handler()
{

}

void init_max30102(i2c_master_bus_handle_t* i2c_bus_handle)
{
    /*ISR setup*/
    gpio_config_t intr_gpio_cfg = 
    {
     .pin_bit_mask = MAX30102_INTR_PIN,
     .mode = GPIO_MODE_INPUT,
     .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&intr_gpio_cfg);
    
    esp_err_t isr_service_err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    /*Check gpio_install_isr_service error code.
    ESP_ERR_INVALID_STATE is acceptable since it signifies that the user already installed an isr before this call.*/
    if(isr_service_err != ESP_ERR_INVALID_STATE && isr_service_err != ESP_OK)
    {
        ESP_ERROR_CHECK(isr_service_err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(MAX30102_INTR_PIN, max30102_isr_handler, NULL));
    
    xTaskCreatePinnedToCore(max30102_buff_service_task, "max30102_buff_service", 4196, i2c_bus_handle, 4, NULL, 1); // Task to read sensor buffer
}
