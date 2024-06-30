#include "ds3231.h"
#include "driver/i2c_master.h"
#include "string.h"
#include "esp_log.h"
#include "time.h"

#define DS3231_I2C_ADDRESS          0x68

#define SECONDS_REGISTER_ADDRESS    0x00
#define MINUTES_REGISTER_ADDRESS    0x01
#define HOURS_REGISTER_ADDRESS      0x02
#define DAY_WEEK_REGISTER_ADDRESS   0x03
#define DAY_MONTH_REGISTER_ADDRESS  0x04
#define MONTH_REGISTER_ADDRESS      0x05
#define YEAR_REGISTER_ADDRESS       0x06


/*TODO:
1.  ADD ERROR HANDLING FOR INVALID TIME INPUT/OUTPUT
    ADD FUNCTION TO AUTOMATICALLY CONVERT __TIME__ __DATE__ TO VALID NUMBERS?
DONE!

2.  Add century functionality as described in the docs
    Check that everything works as expected
    Create new type for specific error codes?
DONE!

optional:
3.  Add alarm functionality
    Add 12h format support
I doubt I'll actually add these since they aren't very useful to me...

FINAL STRETCH:
4.  Add specific error codes - No need
    Seperate the c and header files - DONE
    Add proper doxygen documentation  - DONE
    Create an esp-idf component for the rtc

    ESP_LOGI("", "%i %i %i %i %i %i %i", time_struct.tm_sec, time_struct.tm_min,
    time_struct.tm_hour, time_struct.tm_wday, time_struct.tm_mday, time_struct.tm_mon, time_struct.tm_year);
*/

const char TAG[] = "ds3231";
 
static uint8_t inline bcd_to_decimal(uint8_t bcd_num)
{
    return bcd_num - 6 * (bcd_num >> 4); // Convert an int in bcd format to decimal
}

static uint8_t inline decimal_to_bcd(uint8_t decimal_num)
{
    return (decimal_num%10)+((decimal_num/10)<<4); // Convert an int in decimal format to bcd
}

ds3231_handle_t ds3231_init(i2c_master_bus_handle_t *bus_handle)
{
    /*Initialize the rtc module*/

    // Setup i2c handle
    i2c_device_config_t conf = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_I2C_ADDRESS,
        .scl_speed_hz = 400000
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &conf, &dev_handle));
    
    // Assign relevant information to ds3231 handle
    ds3231_handle_t ds3231_handle;
    ds3231_handle.dev_handle = dev_handle;
    ESP_LOGI(TAG, "ds3231 initialized");

    return ds3231_handle;
}

static esp_err_t validate_time(struct tm *time_struct)
{
    /*Function to check that all the time values are within the expected range*/
    
    if (time_struct->tm_sec > 59 || time_struct->tm_sec < 0)
    {
        ESP_LOGE(TAG, "Invalid seconds value. Expected range: 0<=t<=59, got value: %i instead", time_struct->tm_sec);
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_min > 59 || time_struct->tm_min < 0)
    {
        ESP_LOGE(TAG, "Invalid minutes value. Expected range: 0<=t<=59, got value: %i instead", time_struct->tm_min);
        return ESP_ERR_INVALID_ARG;
    }

    else if ((time_struct->tm_hour > 23 || time_struct->tm_hour < 0))
    {
        ESP_LOGE(TAG, "Invalid hour value. Expected range: 0<=t<=23, got value: %i instead", time_struct->tm_hour);
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_mday > 31 || time_struct->tm_mday < 1)
    {
        ESP_LOGE(TAG, "Invalid day of month value. Expected range: 1<=t<=31, got value: %i instead", time_struct->tm_mday);   
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_mon > 11 || time_struct->tm_mon < 0)
    {
        ESP_LOGE(TAG, "Invalid month value. Expected range: 0<=t<=11, got value: %i instead", time_struct->tm_mon);   
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_year > 199 || time_struct->tm_year < 0)
    {
        ESP_LOGE(TAG, "Invalid year value. Expected range: 0<=t<=199, got value: %i instead", time_struct->tm_year);
        return ESP_ERR_INVALID_ARG;
    }

    else
    {
        return ESP_OK;
    }
}

esp_err_t ds3231_get_datetime(ds3231_handle_t *ds3231_handle, struct tm *time_struct)
{
    /*Get the date and time information from the ds3231*/

    struct tm reference_time; // Creating internal tm struct to validate the ds3231 internal clock before rewriting the passed struct
    bool century_overflow = false;

    int* time_addresses[] = 
    {
        &reference_time.tm_sec,
        &reference_time.tm_min,
        &reference_time.tm_hour,
        &reference_time.tm_wday,
        &reference_time.tm_mday,
        &reference_time.tm_mon,
        &reference_time.tm_year,
    }; // Put all relevant struct pointers into an array for sequential iteration

    uint8_t data_buffer; // Buffer to store returned time byte
    uint8_t register_address; // Data buffer to be sent (contains the register address to be read from)

    int time_addresses_len = sizeof(time_addresses)/sizeof(time_addresses[0]); // Calculate the size of the array

    for (uint8_t i = 0; i < time_addresses_len; i++)
    {
        register_address = i; // The ds3231 time registers are ordered by ascending values (starting from 0 for seconds)
        
        if (i2c_master_transmit_receive(ds3231_handle->dev_handle, &register_address, 1, &data_buffer, 1, -1) == ESP_OK) // Get relevant register info
        {
            if (i == MONTH_REGISTER_ADDRESS && (data_buffer >> 7) == 1){data_buffer -= 0b10000000; century_overflow=true;} // Adjust value to remove century-overflow data
            *time_addresses[i] = (int)bcd_to_decimal(data_buffer); // The data is stored in a bcd so it needs to be converted to decimal 
        }
        else
        {
            ESP_LOGE(TAG, "Couldn't connect to rtc!");
            return ESP_ERR_TIMEOUT;
        }
    }
    // Check if the century bit is high or low and assign the year value accordingly
    if (century_overflow)
    {
        reference_time.tm_year += 100;
    }
    
    reference_time.tm_wday -= 1;
    reference_time.tm_mon -= 1;

    if (validate_time(&reference_time) != ESP_OK)
    {
        ESP_LOGE(TAG, "Recieved invalid time from module! (Possible corruption?)");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Assign unaccounted struct values to preserve them
    reference_time.tm_isdst = time_struct->tm_isdst;
    reference_time.tm_yday = time_struct->tm_yday;

    *time_struct = reference_time; // Override the original time struct

    return ESP_OK;
}

esp_err_t ds3231_validate_time(ds3231_handle_t *ds3231_handle)
{
    struct tm received_time;
    ds3231_get_datetime(ds3231_handle, &received_time); // Get time
    return validate_time(&received_time); // Return error/success code
}

esp_err_t ds3231_set_datetime(ds3231_handle_t *ds3231_handle, struct tm time_struct)
{
    /*Set the date and time of the rtc module*/
    bool century_overflow = false; // Flag to determin if the year is in the range of 2000-2099 or 2100-2199

    if (validate_time(&time_struct) == ESP_OK)
    {
        mktime(&time_struct); // Calculate day of week
        i2c_master_dev_handle_t dev_handle = ds3231_handle->dev_handle;

        if (time_struct.tm_year > 99)
        {
            time_struct.tm_year -= 100;// Udjust for rtc year range (0-99)
            century_overflow = true; // The ds3231 stores year values between 0-99, when the value overflows, the 8th bit of the month register gets flipped 
        } 

        int* time_addresses[] = 
        {
            &time_struct.tm_sec,
            &time_struct.tm_min,
            &time_struct.tm_hour,
            &time_struct.tm_wday,
            &time_struct.tm_mday,
            &time_struct.tm_mon,
            &time_struct.tm_year,
        }; // Put all relevant struct pointers into an array for sequential iteration

        int time_addresses_len = sizeof(time_addresses)/sizeof(time_addresses[0]); // Calculate the size of the array
        
        time_struct.tm_wday += 1;
        time_struct.tm_mon += 1;

        for (uint8_t i = 0; i < time_addresses_len; i++)
        {
            uint8_t buffer[2] = {i, decimal_to_bcd(*time_addresses[i])}; // Convert time to bcd format
            if (i == MONTH_REGISTER_ADDRESS && century_overflow) {buffer[1] += 0b10000000;} // Set month's 8th bit to 1 if there is a century overflow
            i2c_master_transmit(dev_handle, buffer, 2, -1); // Set registers
        }

        return ESP_OK;

    }
    else
    {
        ESP_LOGE(TAG, "Invalid time struct passed; Time was not set...");
        return ESP_ERR_INVALID_ARG;
    }
}

static int str_to_month(const char* month)
{
    /*Convert a month string into a corresponding number*/
    const char* months_of_year[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    for (int i = 0; i < 12; i++)
    {
        if (strcmp(month, months_of_year[i]) == 0)
        {
            return i;
        }   
    }
    return ESP_ERR_INVALID_ARG; 
}



void ds3231_set_datetime_at_compile(ds3231_handle_t *ds3231_handle, bool force_setting)
{    
    /*Set the time during the compilation 
    (Useful for testing or setting the initial time for the module)*/

    struct tm compile_time={.tm_wday=0};
    
    char date_buff[] = __DATE__; // Date during compilation
    char time_buff[] = __TIME__; // Time during compilation
    
    char month[16]; // Temporary buffer to store month string
    
    sscanf(time_buff, "%d:%d:%d", &compile_time.tm_hour, &compile_time.tm_min, &compile_time.tm_sec); // Extract relevant data to struct
    
    sscanf(date_buff, "%s %d %d", month, &compile_time.tm_mday, &compile_time.tm_year); // Extract month to buffer and day and year as intagers
    compile_time.tm_mon = str_to_month(month); // Convert month string to numerical value

    // ESP_LOGI("", "%i %i %i %i %i %i %i", compile_time.tm_sec, compile_time.tm_min,
    // compile_time.tm_hour, compile_time.tm_wday, compile_time.tm_mday, compile_time.tm_mon, compile_time.tm_year);

    compile_time.tm_year -= 1900; // Convert to years since 1900

    mktime(&compile_time); // Calculate day of week

    if (force_setting)
    {
        ds3231_set_datetime(ds3231_handle, compile_time); // Set time on rtc if the time is valid
        ESP_LOGI(TAG, "Successfully set time at compilation!");
    }
    else
    {
        struct tm time_from_rtc;
        ds3231_get_datetime(ds3231_handle, &time_from_rtc);
        
        if(difftime(mktime(&compile_time), mktime(&time_from_rtc)) > 0)
        {
            ds3231_set_datetime(ds3231_handle, compile_time); // Set time on rtc if the time is valid
            ESP_LOGI(TAG, "Successfully set time at compilation!");
        } // If compilation time is newere than rtc time, set time at compilation
        
        else
        {
            ESP_LOGI(TAG, "The time on the rtc is newer that the time at compilation. Time was not set");
        }

    }    
}