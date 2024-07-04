#ifndef MAX30102_H
#define MAX30102_H

#include "driver/i2c_master.h"
#include "max30102_types.h"
#include "max30102_configs.h"

typedef max30102_handle_internal_t* max30102_handle_t;

void max30102_init(i2c_master_bus_handle_t* i2c_bus_handle, max30102_handle_t* return_handle);
esp_err_t max30102_config(max30102_handle_t* sensor_handle, max30102_cfg_t* sensor_config);

#endif // MAX30102_H