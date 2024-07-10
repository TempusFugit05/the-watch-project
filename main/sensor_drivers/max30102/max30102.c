#include "max30102.h"
#include "config_internal.h"
#include "max30102_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define DEBUG_MODE

#define byte_to_binary(num, buff)   for (int i = 0; i < 8; i++) { ((char*)buff)[i] = ((uint8_t)(~((~num << i) | 0b01111111)) == 0b10000000) ? '1' : '0';}

static const char* TAG = "max30102";

static void read_samples(i2c_master_dev_handle_t* dev_handle)
{
        static uint8_t transmit_buff[2] = {FIFO_DATA_REG, 0}; // This will be transmitted to receive a sample from the fifo
        
        static uint8_t data_buff[6] = {0}; // Sample data will be written here
        
        static uint32_t ir_data[32] = {0};
        static uint32_t red_data[32] = {0};
        
        /*Get current read/write pointers*/
        uint8_t READ_PTR_ADDR[2] = {READ_PTR_REG}; // Pointer of fifo sample to read
        uint8_t read_ptr[1];
        
        const uint8_t WRITE_PTR_ADDR[1] = {WRITE_PTR_REG}; // Pointer of fifo sample being written
        uint8_t write_ptr[1];

        ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, WRITE_PTR_ADDR, 1, write_ptr, 1, -1));
        ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, READ_PTR_ADDR, 1, read_ptr, 1, -1));
        READ_PTR_ADDR[1] = read_ptr[0];
        transmit_buff[1] = (read_ptr[0] + 1) % 32; // Update the fifo pointer to read from

        int8_t samples_to_read = write_ptr[0] - read_ptr[0]; // Number of samples available to read in the fifo
        if (samples_to_read <= 0)
        {
            samples_to_read += 32;
        }
        
        uint8_t test[1];
        for (int i = 0; i < samples_to_read; i++)
        {
            /*Break down sample data into the red and ir led readings*/
            transmit_buff[1] = read_ptr[0];
            ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, transmit_buff, 2, data_buff, 6, -1));
            ESP_ERROR_CHECK(i2c_master_transmit(*dev_handle, READ_PTR_ADDR, 2, -1));
            ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, READ_PTR_ADDR, 1, test, 1, -1));
            ESP_LOGI(TAG, "%i", test[0]);

            read_ptr[0] = (read_ptr[0] + 1)%32;
            READ_PTR_ADDR[1] = read_ptr[0];

            red_data[i] = (uint32_t)data_buff[2] << 16 | (uint32_t)data_buff[1] << 8 | (uint32_t)data_buff[0];
            ir_data[i] = (uint32_t)data_buff[5] << 16 | (uint32_t)data_buff[4] << 8 | (uint32_t)data_buff[3];  
        }
        
        ESP_LOGI(TAG, "Completed read of %d samples", samples_to_read);
        
        for (int i = 0; i < samples_to_read; i++)
        {
            ESP_LOGI(TAG, "Sample No: %d - red led: %d | ir led: %d", i, red_data[i], ir_data[i]);
        }
}

static void reset_fifo_state(i2c_master_dev_handle_t* dev_handle)
{
    static const uint8_t WRITE_REG_ADDRESS[2] = {WRITE_PTR_REG, 0};
    static const uint8_t READ_REG_ADDRESS[2] = {READ_PTR_REG, 0};
    static const uint8_t OVERFLOW_REG_ADDRESS[2] = {FIFO_OVERFLOW_REG, 0};

    ESP_ERROR_CHECK(i2c_master_transmit(*dev_handle, WRITE_REG_ADDRESS, 2, -1));
    ESP_ERROR_CHECK(i2c_master_transmit(*dev_handle, READ_REG_ADDRESS, 2, -1));
    ESP_ERROR_CHECK(i2c_master_transmit(*dev_handle, OVERFLOW_REG_ADDRESS, 2, -1));

    #ifdef DEBUG_MODE
    uint8_t returned_write[1];
    uint8_t returned_read[1];
    uint8_t returned_overflow[1];

    ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, READ_REG_ADDRESS, 1, returned_read, 1, -1));
    ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, WRITE_REG_ADDRESS, 1, returned_write, 1, -1));
    ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, OVERFLOW_REG_ADDRESS, 1, returned_overflow, 1, -1));
    ESP_LOGI(TAG, "Read ptr: %d Write ptr: %d Overflow num: %d", returned_read[0], returned_write[0], returned_overflow[0]);
    #endif

    // read_samples(dev_handle);
}

static void max30102_buff_service_task(void* max30102_dev_handle)
{
    max30102_handle_internal_t* max30102_handle = (max30102_handle_internal_t*)max30102_dev_handle;
    SemaphoreHandle_t semaphore = max30102_handle->fifo_semaphore;

    i2c_master_dev_handle_t dev_handle = max30102_handle->sensor_dev_handle;

    static const uint8_t INTERRUPT_REG_ADDRESS[1] = {INTERRUPT_STATUS_REG_1};
    uint8_t interrupt_mask[1] = {0};

    reset_fifo_state(&dev_handle);

    
    while (1)
    {    
        
        /*Wait for sensor buffer to get full*/
        if (xSemaphoreTake(semaphore, portMAX_DELAY))
        {
            ESP_ERROR_CHECK(i2c_master_transmit_receive(max30102_handle->sensor_dev_handle, INTERRUPT_REG_ADDRESS, 1, interrupt_mask, 1, -1));
            
            #ifdef DEBUG_MODE
            char interrupt_bin[9] = {"\0"};
            byte_to_binary(interrupt_mask[0], interrupt_bin);
            ESP_LOGI(TAG, "Received interrupt mask: %s", interrupt_bin);
            #endif

            if (interrupt_mask[0] >> 7 == 1)
            {
                read_samples(&dev_handle);
            }
            
        }
    }
    
}

static IRAM_ATTR void max30102_isr_handler(void* max30102_dev_handle)
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

static uint8_t get_reset_enable_mask(const max30102_cfg_t* config)
{
    uint8_t bit_mask = 0;
    
    if (config->enable_reset == RESET_ENABLE)
    {
        bit_mask |= RESET_ENABLE_MASK;
    }
    else if (config->enable_reset != RESET_DISABLE)
    {
        ESP_LOGE(TAG, "Invalid reset-enable value! Value should be one of reset_enable_t");
    }
    
    return bit_mask;
}
static uint8_t get_interrupt_mask_1(const max30102_cfg_t* config)
{
    /*Mask 1*/
    uint8_t bit_mask = 0;

    const intr_enable_t* interrupts = &config->intr_enable_flags;

    if (interrupts->almost_full_intr)
    {
        bit_mask |= ALMOST_FULL_INTR_MASK;
    }
    
    if (interrupts->ppg_ready_intr)
    {
        bit_mask |= PPG_READY_INTR_MASK;
    }
    
    if (interrupts->alc_overflow_intr)
    {
        bit_mask |= ALC_OVERFLOW_INTR_MASK;
    }
    
    return bit_mask; // Save resulting mask to struct
}

static uint8_t get_interrupt_mask_2(const max30102_cfg_t* config)
{
    const intr_enable_t* interrupts = &config->intr_enable_flags;

    /*Mask 2*/
    uint8_t bit_mask = 0;

    if (interrupts->die_temp_ready_intr)
    {
        bit_mask |= DIE_TEMP_READY_MASK;
    }

    return bit_mask;
}

static uint8_t get_fifo_config_mask(const max30102_cfg_t* config)
{
    int8_t bit_mask = 0;

    if (config->sample_average > FIFO_AVG_SAMPLE_32)
    {
        ESP_LOGE(TAG, "Invalid sample average! Value should be one of sample_average_t");
    }
    else
    {
        bit_mask |= FIFO_AVG_SAMPLE_MASK(config->sample_average);
    }
    
    if (config->fifo_rollover == FIFO_ROLLOVER_ENABLE)
    {
        bit_mask |= FIFO_ROLLOVER_ENABLE_MASK;
    }
    else if (config->fifo_rollover != FIFO_ROLLOVER_DISABLE)
    {
        ESP_LOGE(TAG, "Invalid rollover valude! Value should be one of fifo_rollover_t.");
    }
    
    if (config->fifo_unread_samples_intr > ALMOST_FULL_CFG_MAX)
    {
        ESP_LOGE(TAG, "The fifo fill number to activate an interrupt should be between %d and %d.", ALMOST_FULL_CFG_MIN, ALMOST_FULL_CFG_MAX);
    }
    else
    {
        bit_mask |= config->fifo_unread_samples_intr;
    }   

    return bit_mask;
}

static uint8_t get_mode_config_mask(const max30102_cfg_t* config)
{
    uint8_t bit_mask = 0;

    switch (config->led_mode)
    {
    case LED_HR_ONLY_MODE:
        bit_mask |= LED_HR_ONLY_MODE_MASK;
        break;

    case LED_SPO2_ONLY_MODE:
        bit_mask |= LED_SPO2_ONLY_MODE_MASK;
        break;
    
    case LED_MULTI_LED_MODE:
        bit_mask |= LED_MULTI_LED_MODE_MASK;
        break;

    default:
        ESP_LOGE(TAG, "Invalid LED mode! Value should be one of led_mode_t.");
        break;
    }

    return  bit_mask;
}

static uint8_t get_spo2_config_mask(const max30102_cfg_t* config, uint8_t reserved_bit_mask)
{
    uint8_t bit_mask = 0;
    
    if (SPO2_RANGE_2048 <= config->spo2_range && config->spo2_range <= SPO2_RANGE_16384)
    {
        bit_mask |= SPO2_RANGE_MASK(config->spo2_range);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid SpO2 rang value! Value should be one of spo2_adc_range_t");
    }
    
    if (SPO2_SAMPLE_RATE_50_HZ <= config->spo2_sample_rate && config->spo2_sample_rate <= SPO2_SAMPLE_RATE_3200_HZ)
    {
        bit_mask |= SPO2_SAMPLE_RATE_MASK(config->spo2_sample_rate);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid SpO2 sample rate value! Value should be one of spo2_sample_rate");
    }
    
    if (LED_ADC_RESOLUTION_15_BIT <= config->adc_resolution && config->adc_resolution <= LED_ADC_RESOLUTION_18_BIT)
    {
        bit_mask |= LED_ADC_RESOLUTION_MASK(config->adc_resolution);    }
    else
    {
        ESP_LOGE(TAG, "Invalid LED adc resolution value! Value should be one of led_adc_resolution_t");
    }

    bit_mask |= reserved_bit_mask;

    return bit_mask;
}

static uint8_t get_led_slot_config_mask(const max30102_cfg_t* config)
{
    uint8_t bit_mask = 0;

    switch (config->led_mode)
    {
    case LED_HR_ONLY_MODE:
        bit_mask |= RED_LED_SLOT_MASK;
        break;
    
    case LED_SPO2_ONLY_MODE:
        bit_mask |= IR_LED_SLOT_MASK;
        break;

    case LED_MULTI_LED_MODE:
        bit_mask |= IR_LED_SLOT_MASK << 4;
        bit_mask |= RED_LED_SLOT_MASK;
        break;

    default:
        break;
    }
    
    return bit_mask;
}

static void config_isr(max30102_handle_t* sensor_handle)
{
    /*ISR setup*/
    gpio_config_t intr_gpio_cfg = 
    {
        .pin_bit_mask = 1ULL << GPIO_NUM_4,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&intr_gpio_cfg);
    ESP_ERROR_CHECK(gpio_set_intr_type(GPIO_NUM_4, GPIO_INTR_NEGEDGE));

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

    config_isr((max30102_handle_t*)(&max30102_handle));
}

static void inline config_register(uint8_t reg_addr, uint8_t data, const i2c_master_dev_handle_t dev_handle)
{
    uint8_t buff[2]; // Buffer to send over i2c to save config bits to sensor
    buff[0] = reg_addr;
    buff[1] = data;

    #ifdef DEBUG_MODE
    uint8_t debug_received_data[1] = {0};
    char debug_buff_before_transmit[9] = {'\0'};
    char debug_buff_transmitted[9] = {'\0'};
    char debug_buff_received[9] = {'\0'};
    #endif
    
    i2c_master_transmit(dev_handle, buff, 2, -1);

    #ifdef DEBUG_MODE
    byte_to_binary(debug_received_data[0], debug_buff_before_transmit);
    byte_to_binary(data, debug_buff_transmitted);
    i2c_master_transmit_receive(dev_handle, buff, 1, debug_received_data, 1, -1);
    byte_to_binary(debug_received_data[0], debug_buff_received);
    ESP_LOGI(TAG, "Writing to: 0x%02X, before_transmit: %s, t: %s, r: %s", buff[0],debug_buff_before_transmit, debug_buff_transmitted, debug_buff_received);
    #endif
}

void max30102_config(const max30102_handle_t* sensor_handle, const max30102_cfg_t* sensor_config)
{
    i2c_master_dev_handle_t dev_handle = (*(max30102_handle_internal_t**)sensor_handle)->sensor_dev_handle;
    /*Reset settings*/
    config_register(MODE_CFG_REG, get_reset_enable_mask(sensor_config), dev_handle);

    /*Enable interrupts (first register)*/
    config_register(INTERRUPT_ENABLE_REG_1, get_interrupt_mask_1(sensor_config), dev_handle);

    /*Enable interrupts (second register)*/
    config_register(INTERRUPT_ENABLE_REG_2, get_interrupt_mask_2(sensor_config), dev_handle);

    /*Set FIFO config registers*/
    config_register(FIFO_CFG_REG, get_fifo_config_mask(sensor_config), dev_handle);

    /*Set mode config registers*/
    config_register(MODE_CFG_REG, get_mode_config_mask(sensor_config), dev_handle);

    /*Set SpO2 config registers*/
    /*Get reserved bit to apply to the new mask*/
    uint8_t transmit_buff[1] = {SPO2_CFG_REG};
    uint8_t receive_buff[1] = {0};
    i2c_master_transmit_receive(dev_handle, transmit_buff, 1, receive_buff, 1, -1);
    receive_buff[0] = (receive_buff[0] << 7) >>7;
    config_register(SPO2_CFG_REG, get_spo2_config_mask(sensor_config, receive_buff[0]), dev_handle);

    /*Set red LED brightness*/
    config_register(LED_1_AMPLITUDE_CFG_REG, sensor_config->red_led_amplitude, dev_handle);

    /*Set IR led brightness*/
    config_register(LED_2_AMPLITUDE_CFG_REG, sensor_config->ir_led_amplitude, dev_handle);

    /*Configure led slots*/
    config_register(LED_CONTROL_REG_1, get_led_slot_config_mask(sensor_config), dev_handle);

    /*Disable all other slots*/
    config_register(LED_CONTROL_REG_2, 0, dev_handle);

    xTaskCreatePinnedToCore(max30102_buff_service_task, "max30102_buff_service", 4196, *sensor_handle, 4, NULL, 1); // Task to read sensor buffer
}