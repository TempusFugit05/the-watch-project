#include "driver/i2c_master.h"
#include "string.h"
#include <ctime>

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
    Create new type for specific error codes?

3.  Add alarm functionality (optional(not very useful IMO))
*/

typedef enum{
    DS3231_TIME_FORMAT_24_HOURS,
    DS3231_TIME_FORMAT_12_HOURS
}hour_format_t;

typedef struct
{
    i2c_master_dev_handle_t dev_handle;
    hour_format_t hour_format;
}ds3231_handle_t;

uint8_t inline bcd_to_decimal(uint8_t bcd_num)
{
    return bcd_num - 6 * (bcd_num >> 4); // Convert an int in bcd format to decimal
}

uint8_t inline decimal_to_bcd(uint8_t decimal_num)
{
    return (decimal_num%10)+((decimal_num/10)<<4); // Convert an int in decimal format to bcd
}

void ds3231_config_hour_format(const ds3231_handle_t ds3231_handle, hour_format_t format)
{
    /*Toggle between 12h format and 24h format*/

    i2c_master_dev_handle_t dev_handle = ds3231_handle.dev_handle;

    uint8_t time_buffer[1]; // Buffer to store the recieved time data
    uint8_t register_num_time_buffer[1] = {HOURS_REGISTER_ADDRESS};
    i2c_master_transmit_receive(dev_handle, register_num_time_buffer, 1, time_buffer, 1, -1); // Get hour data

    // The sixth bit in the register toggles between the 12h and 24h formats
    // if bit == 1 -> 12h format
    if(format == DS3231_TIME_FORMAT_12_HOURS)
    {
        *time_buffer = *time_buffer | 0b00100000; // Set sixth bit to 1
    }
    else
    {
        *time_buffer = ~((~*time_buffer) | 0b00100000); // Set sixth bit to 0
    }

    uint8_t transmition_buffer[2] = {*register_num_time_buffer, *time_buffer}; // Rewrite the hour data to includ desired format
    i2c_master_transmit(dev_handle, transmition_buffer, 2, -1);
}

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

int validate_time(ds3231_handle_t *ds3231_handle, tm *time_struct)
{
    /*Function to check that all the time values are within the expected range*/
    char TAG[] = "ds3231";
    
    if (time_struct->tm_sec > 59 or time_struct->tm_sec < 0)
    {
        ESP_LOGE(TAG, "Invalid seconds value. Expected range: 0<=t<=59, got value: %i instead", time_struct->tm_sec);
        return -1;
    }

    else if (time_struct->tm_min > 59 or time_struct->tm_min < 0)
    {
        ESP_LOGE(TAG, "Invalid minutes value. Expected range: 0<=t<=59, got value: %i instead", time_struct->tm_min);
        return -1;
    }

    else if ((time_struct->tm_hour > 24 or time_struct->tm_hour < 0) && ds3231_handle->hour_format == DS3231_TIME_FORMAT_24_HOURS)
    {
        ESP_LOGE(TAG, "Invalid hour value. Expected range (24h format): 0<=t<=24, got value: %i instead", time_struct->tm_hour);
        return -1;
    }

    else if ((time_struct->tm_hour > 12 or time_struct->tm_hour < 0) && ds3231_handle->hour_format == DS3231_TIME_FORMAT_12_HOURS)
    {
        ESP_LOGE(TAG, "Invalid hour value. Expected range (12h format): 0<=t<=12, got value: %i instead", time_struct->tm_hour);
        return -1;
    }

    else if (time_struct->tm_mday > 31 or time_struct->tm_mday < 1)
    {
        ESP_LOGE(TAG, "Invalid day of month value. Expected range: 1<=t<=31, got value: %i instead", time_struct->tm_mday);   
        return -1;
    }

    else if (time_struct->tm_year-2000 > 99 or time_struct->tm_year-2000 < 0)
    {
        ESP_LOGE(TAG, "Invalid year value. Expected range: 0<=t<=99, got value: %i instead", time_struct->tm_year);
        return -1;
    }

    else if (time_struct->tm_wday > 7 or time_struct->tm_wday < 1)
    {
        // The day of week can be manually calculated and therfore returns a different code
        ESP_LOGE(TAG, "Invalid weekday value. Expected range: 1<=t<=7, got value: %i instead", time_struct->tm_wday);
        return -2; 
    }

    else
    {
        return 0;
    }
}

int ds3231_get_datetime(ds3231_handle_t *ds3231_handle, tm *time_struct)
{
    /*Get the date and time information from the ds3231*/

    tm reference_time; // Creating internal tm struct to validate the ds3231 internal clock before rewriting the passed struct

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

    uint8_t buffer[1]; // Buffer to store returned time byte
    uint8_t register_address[1]; // Data buffer to be sent

    int time_addresses_len = sizeof(time_addresses)/sizeof(time_addresses[0]); // Calculate the size of the array
    char* TAG = "ds3231";
    
    for (uint8_t i = 0; i < time_addresses_len; i++)
    {
        register_address[0] = i; // The ds3231 time registers are ordered by ascending values (starting from 0 for seconds)
        i2c_master_transmit_receive(ds3231_handle->dev_handle, register_address, 1, buffer, 1, -1); // Get relevant register info
        *time_addresses[i] = (int)bcd_to_decimal(*buffer); // The data is stored in a bcd so it needs to be converted to decimal 
    }
    reference_time.tm_year += 2000; // The ds3231 stores the year data in a range of 0 to 99
    
    reference_time.tm_hour = (reference_time.tm_hour << 3) >> 3;


    if (validate_time(ds3231_handle, &reference_time) != 0)
    {
        ESP_LOGE(TAG, "Recieved invalid time from module; Discarding changes...");
        return -1;
    }

    *time_struct = reference_time; // Override the original time struct
    return 0;
}

int calculate_day_of_week(int year, int month, int day_of_month)
{
    /*A cool little function I found online to calculate the day of the week!
    NOTE: The original function returns values from 0-6, the has the week day 
    value be between 1-7!*/
    
    year -= month < 3;
    return((year+year/4-year/100+year/400+"-bed=pen+mad."[month]+day_of_month)%7)+1;
}

int ds3231_set_datetime(ds3231_handle_t *ds3231_handle, tm *time_struct)
{
    /*Set the date and time of the rtc module*/
    char TAG[] = "ds3231";
    int time_validation_err_code = validate_time(ds3231_handle, time_struct);

    if (time_validation_err_code == -2 or time_validation_err_code == 0)
    {
        if (time_validation_err_code == -2)
        {
            ESP_LOGW(TAG, "No valid day of week value was passed. Calculating manually...");
            calculate_day_of_week(time_struct->tm_year, time_struct->tm_mon, time_struct->tm_mday);
        } // Calculate the day of week if invalid

        i2c_master_dev_handle_t dev_handle = ds3231_handle->dev_handle;

        time_struct->tm_year -= 2000; // Udjust for rtc year range (0-99)
        
        int* time_addresses[] = 
        {
            &time_struct->tm_sec,
            &time_struct->tm_min,
            &time_struct->tm_hour,
            &time_struct->tm_wday,
            &time_struct->tm_mday,
            &time_struct->tm_mon,
            &time_struct->tm_year,
        }; // Put all relevant struct pointers into an array for sequential iteration

        int time_addresses_len = sizeof(time_addresses)/sizeof(time_addresses[0]); // Calculate the size of the array

        for (uint8_t i = 0; i < time_addresses_len; i++)
        {
            uint8_t buffer[2] = {i, decimal_to_bcd(*time_addresses[i])}; // Convert time to bcd format
            i2c_master_transmit(dev_handle, buffer, 2, -1); // Set registers
        }
        
        time_struct->tm_year += 2000; // Revert changes made to the time struct
        
        return 0;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid time struct passed; Time was not set...");
        return -1;
    }
}

int str_to_month(const char* month)
{
    /*Convert a month string into a corresponding number*/
    const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    int num_monthes = sizeof(months_of_year)/sizeof(months_of_year[0]);
    for (int i = 0; i < num_monthes;i++)
    {
        if (strcmp(month, months_of_year[i]) == 0)
        {
            return i+1;
        }
        
    }
    return -1; 
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
    
    int time_validation_err_code = validate_time(ds3231_handle, &time_struct); // Check the time values and check for errors
    if (time_validation_err_code == 0 or time_validation_err_code == -2)
    {
        time_struct.tm_wday = calculate_day_of_week(time_struct.tm_year, time_struct.tm_mon, time_struct.tm_mday); // Manually calculate the day of the week
        ESP_LOGI(TAG, "Manually calculating the day of week");
        ds3231_set_datetime(ds3231_handle, &time_struct); // Set time on rtc if the time is valid
        ESP_LOGI(TAG, "Successfully set time at compilation!");
    }

    else
    {
        ESP_LOGE(TAG, "Couldn't set the time!"); // Error if something went terribly wrong
    }
    
}