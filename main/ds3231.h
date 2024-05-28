#ifndef DS3231_H
#define DS3231_H

#include "driver/i2c_master.h"
#include "stdlib.h"
#include "time.h"

// DS3231 I2C address
#define DS3231_I2C_ADDRESS 0x68

// Register addresses
#define SECONDS_REGISTER_ADDRESS    0x00
#define MINUTES_REGISTER_ADDRESS    0x01
#define HOURS_REGISTER_ADDRESS      0x02
#define DAY_WEEK_REGISTER_ADDRESS   0x03
#define DAY_MONTH_REGISTER_ADDRESS  0x04
#define MONTH_REGISTER_ADDRESS      0x05
#define YEAR_REGISTER_ADDRESS       0x06

// DS3231 handle structure
typedef struct
{
    i2c_master_dev_handle_t dev_handle;
} ds3231_handle_t;

// Function prototypes

/**
 * @brief Initialize the ds3231
 * @param bus_handle Pointer to the I2C master bus handle
 * @return A ds3231 handle
 */
ds3231_handle_t ds3231_init(i2c_master_bus_handle_t *bus_handle);

/**
 * @brief Get the date and time stored in the ds3231 and write it to a tm struct
 * @param ds3231_handle A pointer to a ds3231 handle
 * @param time_struct A pointer to a tm struct into which the time information will be written
 * @return ESP_OK if the time read from the module is valid, ESP_ERR_INVALID_ARG if the time read is invalid
 */
int ds3231_get_datetime(ds3231_handle_t *ds3231_handle, struct tm *time_struct);

/**
 * @brief Set the date and time of the ds3231
 * @param ds3231_handle A pointer to a ds3231 handle
 * @param time_struct A tm struct containing the date and time to set
 * @return ESP_OK if the time was set successfully, ESP_ERR_INVALID_ARG if the time is invalid
 */
int ds3231_set_datetime(ds3231_handle_t *ds3231_handle, struct tm time_struct);

/**
 * @brief Set the time on the ds3231 at the time of compilation
 * The time will only be set if the compilation time is newer than the time stored on the ds3231 (unless force_setting is set to true)
 * @param ds3231_handle A pointer to a ds3231 handle
 * @param force_setting Force the time to be set
 */
int ds3231_set_datetime_at_compile(ds3231_handle_t *ds3231_handle, i2c_master_bus_handle_t *i2c_bus_handle, bool force_setting);

/**
 * @brief Convert a BCD number to a decimal number
 * @param bcd_num BCD number
 * @return Decimal number
 */
uint8_t bcd_to_decimal(uint8_t bcd_num);

/**
 * @brief Convert a decimal number to a BCD number
 * @param decimal_num Decimal number
 * @return BCD number
 */
uint8_t decimal_to_bcd(uint8_t decimal_num);

/**
 * @brief Validate the time stored in a tm struct
 * @param ds3231_handle A pointer to a ds3231 handle
 * @param time_struct A pointer to a tm struct containing the time to validate
 * @return ESP_OK if the time is valid, ESP_ERR_INVALID_ARG if the time is invalid
 */
int validate_time(ds3231_handle_t *ds3231_handle, struct tm *time_struct);

/**
 * @brief Calculate the day of the week for a given date
 * @param year The year
 * @param month The month
 * @param day_of_month The day of the month
 * @return The day of the week (1-7)
 */
int calculate_day_of_week(int year, int month, int day_of_month);

/**
 * @brief Convert a month string to a numerical value
 * @param month Month string
 * @return The numerical value of the month (1-12), or ESP_ERR_INVALID_ARG if the month is invalid
 */
int str_to_month(const char *month);

#endif // DS3231_H