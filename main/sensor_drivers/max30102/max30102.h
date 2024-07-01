#ifndef MAX30102_H
#define MAX30102_H

#include "driver/i2c_master.h"

void init_max30102(i2c_master_bus_handle_t* i2c_bus_handle);

#endif // MAX30102_H