#include "max30102.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define MAX30102_I2C_ADDR   0x57
#define MAX30102_INTR_PIN   GPIO_NUM_4 // Interrupt to signify fifo buffer almost full (active-low)

#define FIFO_DATA_REG       0x7
#define READ_PTR_REG        0x6
#define WRITE_PTR_REG       0x4

typedef struct
{
    SemaphoreHandle_t fifo_semaphore;
    i2c_master_dev_handle_t sensor_dev_handle;
} max30102_handle_t;


static void max30102_buff_service_task(void* max30102_dev_handle)
{
    max30102_handle_t* max30102_handle = (max30102_handle_t*)max30102_dev_handle;

    i2c_master_dev_handle_t dev_handle = max30102_handle->sensor_dev_handle;
    SemaphoreHandle_t fifo_semaphore = max30102_handle->fifo_semaphore;

    vTaskDelay(pdMS_TO_TICKS(500));

    const uint8_t READ_REG_ADDR[1] = {READ_PTR_REG};
    const uint8_t WRITE_REG_ADDR[1] = {WRITE_PTR_REG};

    uint8_t read_ptr[1]; // Pointer of fifo sample to read
    uint8_t write_ptr[1]; // Pointer of fifo sample being written

    /*Get current read/write pointers*/
    i2c_master_transmit_receive(dev_handle, READ_REG_ADDR, 1, read_ptr, 1, -1);
    i2c_master_transmit_receive(dev_handle, WRITE_REG_ADDR, 1, write_ptr, 1, -1);

    uint8_t samples_to_read = (write_ptr[0] - read_ptr[0]) % 32; // Number of samples available to read in the fifo

    uint8_t transmit_buff[2] = {FIFO_DATA_REG, read_ptr[0]}; // This will be transmitted to receive a sample from the fifo
    uint8_t ptr_incrament_buff[2] = {READ_PTR_REG, 0};
    
    uint8_t data_buff[6] = {0}; // Sample data will be written here

    uint32_t red_led;
    uint32_t ir_red;

    for (int i = 0; i < samples_to_read; i++)
    {
        i2c_master_transmit_receive(dev_handle, transmit_buff, 2, data_buff, 6, -1);

        /*Break down sample data into the red and ir led readings*/
        red_led = (uint32_t)data_buff[2] << 16 | (uint32_t)data_buff[1] << 8 | (uint32_t)data_buff[0];
        ir_red = (uint32_t)data_buff[5] << 16 | (uint32_t)data_buff[4] << 8 | (uint32_t)data_buff[3];
        
        ESP_LOGI("", "Reading from: %i - red led: %d | ir led: %d", read_ptr[0], red_led, ir_red);

        read_ptr[0] = (read_ptr[0] + 1) % 32; // Incrament fifo address being read from
        transmit_buff[1] = read_ptr[0];
        ptr_incrament_buff[1] = read_ptr[0];
        i2c_master_transmit(dev_handle, ptr_incrament_buff, 2, -1);
    }
    
    ESP_LOGI("", "Completed read of %d samples", samples_to_read);
    
    while (1)
    {    
        /*Wait for sensor buffer to get full*/
        if (xSemaphoreTake(fifo_semaphore, portMAX_DELAY))
        {
            ESP_LOGI("success", "");
        }      
    }
    
}

IRAM_ATTR void max30102_isr_handler(void* max30102_dev_handle)
{
    max30102_handle_t* dev_handle = (max30102_handle_t*)max30102_dev_handle;

    SemaphoreHandle_t fifo_semaphore = dev_handle->fifo_semaphore;

    BaseType_t should_yield = pdFALSE;
    
    xSemaphoreGiveFromISR(fifo_semaphore, &should_yield);

    if (should_yield != pdFALSE)
    {
       portYIELD_FROM_ISR();
    }
}

void init_max30102(i2c_master_bus_handle_t* i2c_bus_handle)
{
    /*Struct to hold data needed for sensor communication*/
    static max30102_handle_t max30102_handle;
    max30102_handle.fifo_semaphore = xSemaphoreCreateBinary();

    i2c_device_config_t dev_cfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDR,
        .scl_speed_hz = 400000
    };
    i2c_master_bus_add_device(*i2c_bus_handle, &dev_cfg, &max30102_handle.sensor_dev_handle);
    
    /*ISR setup*/
    gpio_config_t intr_gpio_cfg = 
    {
        .pin_bit_mask = 1ULL << MAX30102_INTR_PIN,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&intr_gpio_cfg);
    ESP_ERROR_CHECK(gpio_set_intr_type(MAX30102_INTR_PIN, GPIO_INTR_ANYEDGE));

    // gpio_isr_register();
    esp_err_t isr_service_err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    /*Check gpio_install_isr_service error code.
    ESP_ERR_INVALID_STATE is acceptable since it signifies that the user already installed an isr before this call.*/
    if(isr_service_err != ESP_ERR_INVALID_STATE && isr_service_err != ESP_OK)
    {
        ESP_ERROR_CHECK(isr_service_err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(MAX30102_INTR_PIN, max30102_isr_handler, &max30102_handle));
    
    if (i2c_master_probe(*i2c_bus_handle, MAX30102_I2C_ADDR, 1000) != ESP_OK)
    {
        ESP_LOGE("", "Sensor not connected...");
    }

    uint8_t config_buff[2];

    /*Interrupt enable*/
    config_buff[0] = 0x02;
    config_buff[1] = 0b10100001; // Enable almost full, ambient light and power ready interrupts
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    /*Led config*/
    config_buff[0] = 0x09;
    config_buff[1] = 0b01000111; // Disable shut-down, reset power mode, set to red+ir led mode
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    /*Fifo config*/
    config_buff[0] = 0x08;
    config_buff[1] = 0b10010000; // 16 sample average, rollover enabled, generate interrupt on 0 read samples
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    /*Clear fifo pointers and overflow*/
    config_buff[0] = 0x4;
    config_buff[1] = 0;
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    config_buff[0] = 0x05;
    config_buff[1] = 0;
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    config_buff[0] = 0x7;
    config_buff[1] = 0;
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    /*Led amplitude config*/
    config_buff[0] = 0x0C;
    config_buff[1] = 0xFF; // Around 50 ma
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    config_buff[0] = 0x0D;
    config_buff[1] = 0xFF; // Around 50 ma
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    /*slots config*/
    config_buff[0] = 0x11;
    config_buff[1] = 0b00100010; // Enable red and ir
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    config_buff[0] = 0x12;
    config_buff[1] = 0; // Disable other slots
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    /*SPO2 config*/
    config_buff[0] = 0x0A;
    config_buff[1] = 0b01100111; // Set range to 16384, 100 samples/s, 18 bit resolution
    i2c_master_transmit(max30102_handle.sensor_dev_handle, config_buff, 2, -1);

    xTaskCreatePinnedToCore(max30102_buff_service_task, "max30102_buff_service", 4196, &max30102_handle, 4, NULL, 1); // Task to read sensor buffer
}
