#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H

#include "widget.h"

class test_widget : public widget
{
    public:
        test_widget(hagl_backend_t* display, SemaphoreHandle_t  display_mutex, hagl_window_t clip);
        ~test_widget() override;
        void run_widget() override;
};

#endif // TEST_WIDGET_H