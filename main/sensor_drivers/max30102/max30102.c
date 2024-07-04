#include "max30102.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static void max30102_buff_service_task(void* max30102_dev_handle)
{
    max30102_handle_internal_t* max30102_handle = (max30102_handle_internal_t*)max30102_dev_handle;

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
    max30102_handle_internal_t* dev_handle = (max30102_handle_internal_t*)max30102_dev_handle;

    SemaphoreHandle_t fifo_semaphore = dev_handle->fifo_semaphore;

    BaseType_t should_yield = pdFALSE;
    
    xSemaphoreGiveFromISR(fifo_semaphore, &should_yield);

    if (should_yield != pdFALSE)
    {
       portYIELD_FROM_ISR();
    }
}

uint8_t get_interrupt_mask_1(intr_enable_t* interrupts)
{
    uint8_t almost_full_intr = 0;
    uint8_t ppg_ready_intr = 0;
    uint8_t alc_overflow_intr = 0;
    uint8_t power_ready_intr = 0;

    if (interrupts->almost_full_intr)
    {
        almost_full_intr = ALMOST_FULL_INTR_MASK;
    }
    
    if (interrupts->ppg_ready_intr)
    {
        ppg_ready_intr = PPG_READY_INTR_MASK;
    }
    
    if (interrupts->alc_overflow_intr)
    {
        alc_overflow_intr = ALC_OVERFLOW_INTR_MASK;
    }
    
    if (interrupts->power_ready_intr)
    {
        power_ready_intr = POWER_READY_INTR_MASK;
    }
    
    return (almost_full_intr | ppg_ready_intr | alc_overflow_intr | power_ready_intr);
}

uint8_t get_interrupt_mask_2(intr_enable_t* interrupts)
{
    uint8_t die_temp_ready_intr = 0;
    if (interrupts->die_temp_ready_intr)
    {
        return DIE_TEMP_READY_MASK;
    }
    return die_temp_ready_intr;
}

void config_isr(max30102_handle_t* sensor_handle)
{
    /*ISR setup*/
    gpio_config_t intr_gpio_cfg = 
    {
        .pin_bit_mask = 1ULL << GPIO_NUM_4,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&intr_gpio_cfg);
    ESP_ERROR_CHECK(gpio_set_intr_type(GPIO_NUM_4, GPIO_INTR_ANYEDGE));

    esp_err_t isr_service_err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    
    /*Check gpio_install_isr_service error code.
    ESP_ERR_INVALID_STATE is acceptable since it signifies that the user already installed an isr before this call.*/
    if(isr_service_err != ESP_ERR_INVALID_STATE && isr_service_err != ESP_OK)
    {
        ESP_ERROR_CHECK(isr_service_err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_4, max30102_isr_handler, sensor_handle));
}

void max30102_init(i2c_master_bus_handle_t* i2c_bus_handle, max30102_handle_t* return_handle)
{
    /*Struct to hold data needed for sensor communication*/
    static max30102_handle_internal_t max30102_handle;
    max30102_handle.fifo_semaphore = xSemaphoreCreateBinary();

    i2c_device_config_t dev_cfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDR,
        .scl_speed_hz = 400000
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*i2c_bus_handle, &dev_cfg, &max30102_handle.sensor_dev_handle));

    *return_handle = (max30102_handle_t)(&max30102_handle);

    xTaskCreatePinnedToCore(max30102_buff_service_task, "max30102_buff_service", 4196, &max30102_handle, 4, NULL, 1); // Task to read sensor buffer
}

void static inline config_register(uint8_t* buff, uint8_t reg_addr, uint8_t data, max30102_handle_t* handle)
{
    buff[0] = reg_addr;
    buff[1] = data;
    i2c_master_transmit((*(max30102_handle_internal_t**)handle)->sensor_dev_handle, buff, 2, -1);
}

esp_err_t max30102_config(max30102_handle_t* sensor_handle, max30102_cfg_t* sensor_config)
{
    uint8_t buff_to_send[2];
    config_internal_t config;
    config.intr_mask_1 = get_interrupt_mask_1(&sensor_config->intr_enable_flags);
    config.intr_mask_2 = get_interrupt_mask_2(&sensor_config->intr_enable_flags);
    config.user_config = sensor_config;

    config_register(buff_to_send, INTERRUPT_ENABLE_REG_1, config.intr_mask_1, sensor_handle);
    config_register(buff_to_send, INTERRUPT_ENABLE_REG_2, config.intr_mask_2, sensor_handle);

    return ESP_OK;
}