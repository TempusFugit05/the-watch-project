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
    app();
    virtual ~app();
    virtual bool get_update_status();
    virtual void run_app();
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
    ~clock_app();

    TaskHandle_t set_task_handle(const TaskHandle_t app_task_handle);
    TaskHandle_t get_task_handle();

    bool get_update_status() override;
    void run_app () override;

};

class test_app : public app
{
    private:
        hagl_backend_t* display_handle;
        TaskHandle_t task_handle;

    public:
        test_app(hagl_backend_t* app_display);
        ~test_app();

        bool get_update_status();
        void run_app ();
        
        TaskHandle_t set_task_handle(const TaskHandle_t app_task_handle);
        TaskHandle_t get_task_handle();
};

#ifdef __cplusplus
}
#endif

#endif