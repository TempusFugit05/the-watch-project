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

class app
{    
protected:
    hagl_backend_t* display_handle;
    TaskHandle_t task_handle;

public:
    app(hagl_backend_t* display);
    virtual ~app() = 0;
    virtual bool get_update_status() = 0;
    virtual void run_app() = 0;
};

class clock_app : public app
{
    private:
    tm reference_time; // Older time to compare against
    tm* current_time; // Newest time available

    char time_str [64]; // Buffer to display the time 
    wchar_t formatted_str [64]; // String buffer compatable with the display library
    char month[10]; // Buffer to store the month's name
    char weekday[10]; // Buffer to store the week's name

    void month_to_str(int month, char* buffer);
    void weekday_to_str(int weekday, char* buffer);
    
    bool first_run = true;

    int hours_text_cords_y;
    int minutes_text_cords_y;
    int seconds_text_cords_y;
    int months_text_cords_y;
    int weekday_text_cords_y;

    public:
    clock_app(tm* time, hagl_backend_t* app_display);
    ~clock_app();

    bool get_update_status() override;
    void run_app () override;

};

class test_app : public app
{
    public:
        test_app(hagl_backend_t* app_display);
        ~test_app() override;

        bool get_update_status() override;
        void run_app () override;
};

#ifdef __cplusplus
}
#endif

#endif