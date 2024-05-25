#ifndef DS3231_H
#define DS3231_H

#include "driver/i2c_master.h"
#include "string.h"
#include <ctime>

// #define SUPPORT_12H_FORMAT

#define DS3231_I2C_ADDRESS          0x68
#define SECONDS_REGISTER_ADDRESS    0x00
#define MINUTES_REGISTER_ADDRESS    0x01
#define HOURS_REGISTER_ADDRESS      0x02
#define DAYS_REGISTER_ADDRESS       0x03
#define YEARS_REGISTER_ADDRESS      0x04


/*TODO:
1.  ADD ERROR HANDLING FOR INVALID TIME INPUT/OUTPUT
    ADD FUNCTION TO AUTOMATICALLY CONVERT __TIME__ __DATE__ TO VALID NUMBERS?
DONE!

2.  Add century functionality as described in the docs
    Check that everything works as expected
    Create new type for specific error codes? - DONE! (Using esp specific error codes)

3.  Add alarm functionality (optional(not very useful IMO))
*/
#ifdef SUPPORT_12H_FORMAT
typedef enum{
    DS3231_TIME_FORMAT_24_HOURS,
    DS3231_TIME_FORMAT_12_HOURS
}hour_format_t;

typedef enum{
    DS3231_HOUR_MIDDAY_AM,
    DS3231_HOUR_MIDDAY_PM
}midday_state_t;
#endif

typedef struct
{
    i2c_master_dev_handle_t dev_handle;
#ifdef SUPPORT_12H_FORMAT
    hour_format_t hour_format;
#endif
}ds3231_handle_t;

uint8_t inline bcd_to_decimal(uint8_t bcd_num)
{
    return bcd_num - 6 * (bcd_num >> 4); // Convert an int in bcd format to decimal
}

uint8_t inline decimal_to_bcd(uint8_t decimal_num)
{
    return (decimal_num%10)+((decimal_num/10)<<4); // Convert an int in decimal format to bcd
}

#ifdef SUPPORT_12H_FORMAT
int ds3231_config_hour_format(ds3231_handle_t *ds3231_handle, hour_format_t format)
{
    /*Toggle between 12h format and 24h format*/
    char TAG[] = "ds3231";
    i2c_master_dev_handle_t dev_handle = ds3231_handle->dev_handle;

    uint8_t data_buffer; // Buffer to store returned time byte
    uint8_t register_address = HOURS_REGISTER_ADDRESS;

    if (i2c_master_transmit_receive(dev_handle, &register_address, 1, &data_buffer, 1, -1) == ESP_OK)
    {// Get hour data
    
        // The sixth bit in the register toggles between the 12h and 24h formats
        // if bit == 1 -> 12h format
        if(format == DS3231_TIME_FORMAT_12_HOURS)
        {
            data_buffer = data_buffer | 0b00100000; // Set sixth bit to 1
        }
        else
        {
            data_buffer = ~((~data_buffer) | 0b00100000); // Set sixth bit to 0
        }

        uint8_t transmition_buffer[2] = {register_address, data_buffer}; // Rewrite the hour data to includ desired format
        
        if (i2c_master_transmit(dev_handle, transmition_buffer, 2, -1) == ESP_OK)
        {
            return ESP_OK;
        }

        else
        {
            goto format_error; // Throw error if rtc is unreachable
        }
    }

    else
    {
format_error:
        ESP_LOGE(TAG, "Could not set hour format!");
        return ESP_ERR_TIMEOUT; // Throw error if rtc is unreachable
    }
}
#endif

#ifdef SUPPORT_12H_FORMAT
int ds3231_get_am_pm(ds3231_handle_t *ds3231_handle)
{
    if (ds3231_handle->hour_format != DS3231_TIME_FORMAT_12_HOURS)
    {
        ESP_LOGE("ds3231", "Can't get am or pm state with the ds3231 configured to a 24 hour format!");
        return ESP_ERR_INVALID_ARG;
    }
    else
    {
        uint8_t register_address = HOURS_REGISTER_ADDRESS;
        uint8_t data_buffer;
        i2c_master_transmit_receive(ds3231_handle->dev_handle, &register_address, sizeof(register_address), &data_buffer, sizeof(data_buffer), -1);
        
        // The 5th bit checks whether or not the time of day is AM or PM when the ds3231 is configured with 12 hour format.
        if (((data_buffer << 2) >> 7) == 1){return DS3231_HOUR_MIDDAY_PM;}
        else{return DS3231_HOUR_MIDDAY_AM;}
    }
}
#endif

#ifdef SUPPORT_12H_FORMAT
ds3231_handle_t ds3231_init(i2c_master_bus_handle_t *bus_handle, hour_format_t time_format)
{
    /*Initialize the rtc module*/

    // Setup i2c handle
    i2c_device_config_t conf = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_I2C_ADDRESS,
        .scl_speed_hz = 100000
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &conf, &dev_handle));
    
    // Assign relevant information to ds3231 handle
    ds3231_handle_t ds3231_handle;
    ds3231_handle.dev_handle = dev_handle;
    ds3231_handle.hour_format = time_format;
    char* TAG = "ds3231";
    ESP_LOGI(TAG, "ds3231 initialized");

    return ds3231_handle;
}
#endif

#ifndef SUPPORT_12H_FORMAT
ds3231_handle_t ds3231_init(i2c_master_bus_handle_t *bus_handle)
{
    /*Initialize the rtc module*/

    // Setup i2c handle
    i2c_device_config_t conf = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_I2C_ADDRESS,
        .scl_speed_hz = 100000
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &conf, &dev_handle));
    
    // Assign relevant information to ds3231 handle
    ds3231_handle_t ds3231_handle;
    ds3231_handle.dev_handle = dev_handle;
    char* TAG = "ds3231";
    ESP_LOGI(TAG, "ds3231 initialized");

    return ds3231_handle;
}
#endif

int validate_time(ds3231_handle_t *ds3231_handle, tm *time_struct)
{
    /*Function to check that all the time values are within the expected range*/
    char TAG[] = "ds3231";
    
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

    else if ((time_struct->tm_hour > 24 || time_struct->tm_hour < 0))
    {
        ESP_LOGE(TAG, "Invalid hour value. Expected range: 0<=t<=24, got value: %i instead", time_struct->tm_hour);
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_mday > 31 || time_struct->tm_mday < 1)
    {
        ESP_LOGE(TAG, "Invalid day of month value. Expected range: 1<=t<=31, got value: %i instead", time_struct->tm_mday);   
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_year-2000 > 199 || time_struct->tm_year-2000 < 0)
    {
        ESP_LOGE(TAG, "Invalid year value. Expected range: 0<=t<=99, got value: %i instead", time_struct->tm_year);
        return ESP_ERR_INVALID_ARG;
    }

    else if (time_struct->tm_wday > 7 || time_struct->tm_wday < 1)
    {
        // The day of week can be manually calculated and therfore returns a different code
        ESP_LOGE(TAG, "Invalid weekday value. Expected range: 1<=t<=7, got value: %i instead", time_struct->tm_wday);
        return -2; 
    }

    else
    {
        return ESP_OK;
    }
}

int ds3231_get_datetime(ds3231_handle_t *ds3231_handle, tm *time_struct)
{
    /*Get the date and time information from the ds3231*/

    tm reference_time; // Creating internal tm struct to validate the ds3231 internal clock before rewriting the passed struct
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
    const char TAG[] = "ds3231";

    for (uint8_t i = 0; i < time_addresses_len; i++)
    {
        register_address = i; // The ds3231 time registers are ordered by ascending values (starting from 0 for seconds)
        
        if (i2c_master_transmit_receive(ds3231_handle->dev_handle, &register_address, 1, &data_buffer, 1, -1) == ESP_OK) // Get relevant register info
        {
            if (i == 5 && (data_buffer >> 7) == 1){data_buffer -= 0b10000000; century_overflow=true;} // Adjust value to remove century-overflow data
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
        reference_time.tm_year += 2100;
    }
    else
    {
        reference_time.tm_year += 2000;
    }
    
    #ifdef SUPPORT_12H_FORMAT
    reference_time.tm_hour = (reference_time.tm_hour << 3) >> 3; // If 12 hour support is enabled, remove am/pm data
    #endif
    
    if (validate_time(ds3231_handle, &reference_time) != ESP_OK)
    {
        ESP_LOGE(TAG, "Recieved invalid time from module (Possible corruption?); Discarding changes...");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Assign unaccounted struct values to preserve them
    reference_time.tm_isdst = time_struct->tm_isdst;
    reference_time.tm_yday = time_struct->tm_yday;

    *time_struct = reference_time; // Override the original time struct
    return ESP_OK;
}

int calculate_day_of_week(int year, int month, int day_of_month)
{
    /*A cool little function I found online to calculate the day of the week!
    NOTE: The weekday range is 1-7*/
    
    year -= month < 3;
    return((year+year/4-year/100+year/400+"-bed=pen+mad."[month]+day_of_month)%7)+1;
}

int ds3231_set_datetime(ds3231_handle_t *ds3231_handle, tm time_struct)
{
    /*Set the date and time of the rtc module*/
    bool century_overflow = false; // Flag to determin if the year is in the range of 2000-2099 or 2100-2199
    char TAG[] = "ds3231";
    time_struct.tm_wday = calculate_day_of_week(time_struct.tm_year, time_struct.tm_mon, time_struct.tm_mday); // Calculate day of week

    if (validate_time(ds3231_handle, &time_struct) == ESP_OK)
    {
        i2c_master_dev_handle_t dev_handle = ds3231_handle->dev_handle;

        #ifdef SUPPORT_12H_FORMAT
        midday_state_t am_pm_state;
        if (time_struct.tm_hour > 11){am_pm_state = DS3231_HOUR_MIDDAY_PM}
        #endif

        if (time_struct.tm_year > 2099)
        {
            time_struct.tm_year -= 2100;// Udjust for rtc year range (0-99)
            century_overflow = true; // The ds3231 stores year values between 0-99, when the value overflows, the 8th bit of the month register gets flipped 
        } 

        else
        {
            time_struct.tm_year -= 2000; // Udjust for rtc year range (0-99)
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

        for (uint8_t i = 0; i < time_addresses_len; i++)
        {
            uint8_t buffer[2] = {i, decimal_to_bcd(*time_addresses[i])}; // Convert time to bcd format
            if (i==5 && century_overflow) {buffer[1] += 0b10000000;} // Set month's 8th bit to 1 if there is a century overflow
            i2c_master_transmit(dev_handle, buffer, 2, -1); // Set registers
        }
        #ifdef SUPPORT_12H_FORMAT
        ds3231_config_hour_format(ds3231_handle, ds3231_handle->hour_format);
        #endif

        return ESP_OK;

    }
    else
    {
        ESP_LOGE(TAG, "Invalid time struct passed; Time was not set...");
        return ESP_ERR_INVALID_ARG;
    }
}

int str_to_month(const char* month)
{
    /*Convert a month string into a corresponding number*/
    const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    
    for (int i = 0; i < 12; i++)
    {
        if (strcmp(month, months_of_year[i]) == 0)
        {
            return i+1;
        }   
    }
    return ESP_ERR_INVALID_ARG; 
}

void ds3231_set_datetime_at_compile(ds3231_handle_t *ds3231_handle)
{
    /*Set the time during the compilation 
    (Useful for testing or setting the initial time for the module)*/
    char TAG[] = "ds3231";

    tm time_struct;
    
    char date_buff[] = __DATE__; // Date during compilation
    char time_buff[] = __TIME__; // Time during compilation
    
    char month[16]; // Temporary buffer to store month string
    
    sscanf(time_buff, "%d:%d:%d", &time_struct.tm_hour, &time_struct.tm_min, &time_struct.tm_sec); // Extract relevant data to struct
    
    sscanf(date_buff, "%s %d %d", month, &time_struct.tm_mday, &time_struct.tm_year); // Extract month to buffer and day and year as intagers
    time_struct.tm_mon = str_to_month(month); // Convert month string to numerical value
    time_struct.tm_wday = calculate_day_of_week(time_struct.tm_year, time_struct.tm_mon, time_struct.tm_mday); // Manually calculate the day of the week
    int time_validation_err_code = validate_time(ds3231_handle, &time_struct); // Check the time values and check for errors
    if (time_validation_err_code == ESP_OK)
    {
        ds3231_set_datetime(ds3231_handle, time_struct); // Set time on rtc if the time is valid
        ESP_LOGI(TAG, "Successfully set time at compilation!");
    }

    else
    {
        ESP_LOGE(TAG, "Couldn't set the time!"); // Error if something went terribly wrong
    }
    
}
#endif // DS3231_H