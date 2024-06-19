#ifndef INFO_BAR_WIDGET_H
#define INFO_BAR_WIDGET_H
#include "widget.h"

class info_bar_widget : public widget
{
    private:
        tm reference_time;
        ds3231_handle_t* rtc_handle;

        char time_str[32];
        wchar_t formatted_time_str[32];
        bool first_run = true;
        
    public:
        info_bar_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t clip, ds3231_handle_t* rtc);
        ~info_bar_widget() override;
        void run_widget() override;
};

#endif // INFO_BAR_WIDGET_H