#ifndef APPS_H
#define APPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ctime>

#include "hagl_hal.h"
#include "hagl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct
{
    int x_min;
    int x_max;
    int y_min;
    int y_max;
} bounding_box_t;

class app
{
public:
    virtual bool get_update_status() = 0;
    virtual void run_app() = 0;
};

class clock_app : public app
{
    private:
    tm reference_time; // Older time to compare against
    tm* current_time; // Newest time available
    bounding_box_t bounding_box; 
    hagl_backend_t* display_handle;
    TaskHandle_t task_handle;

    char time_str [64]; // String buffer
    wchar_t formatted_str [64]; // String buffer compatable with the display library
    
    void month_to_str(int month, char* buffer);
    void weekday_to_str(int weekday, char* buffer);
    
    public:
    clock_app(tm* time, bounding_box_t bounding_box, hagl_backend_t* app_display);
    bool get_update_status() override;
    void run_app () override;
};

#ifdef __cplusplus
}
#endif

#endif