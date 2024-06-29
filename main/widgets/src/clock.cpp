#include "widget.h"
#include "clock_widget.h"

#include "fonts.h"
#include <cstring>
#include"esp_log.h"

#include "lvgl.h"

#define TEXT_COLOR 255, 100, 255
#define CALC_STR_X_POS(clip, str, font, scale_pct) ((clip.x1 - clip.x0) - strlen(str)*font.size_x*scale_pct/100)/2

clock_widget::clock_widget(hagl_backend_t* widget_display, SemaphoreHandle_t display_mutex, hagl_window_t clip, ds3231_handle_t* rtc)
: widget(widget_display, display_mutex, clip, "clock_widget"), rtc_handle(rtc)
{
    ds3231_get_datetime(rtc_handle, &reference_time);
    xTaskCreate(call_run_widget, WIDGET_NAME, 4096, this, 3, &task_handle); // Create task to call run_widget
}

clock_widget::~clock_widget()
{
    // START_WIDGET_DELETION(task_handle, task_deletion_mutex);
    // CLEAR_SCREEN(display_handle, clip, display_mutex);
}

void clock_widget::month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer*/
    static const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    strcpy(buffer, months_of_year[month]); // Copy the month string into the buffer
}

void clock_widget::weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer*/
    static const char* days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday]); // Copy the day of week string into the buffer
}

void clock_widget::display_hours(tm current_time)
{
    if (current_time.tm_hour != reference_time.tm_hour || first_run)
    {
        lv_obj_t* hour_text = lv_label_create(lv_scr_act());
        lv_label_set_text_fmt(hour_text, "%02d", current_time.tm_hour);
        lv_obj_align(hour_text, LV_ALIGN_CENTER, 0, -40);
        hagl_put_text(display_handle, formatted_str, CALC_STR_X_POS(clip, time_str, segment_font, 100),
        hours_text_cords_y, text_color, segment_font.font_data);
    }
}

void clock_widget::display_minutes(tm current_time)
{
    if (current_time.tm_min != reference_time.tm_min || first_run)
    {
        snprintf(time_str, 64, "%02i", current_time.tm_min);
        mbstowcs(formatted_str, time_str, 64);
        minutes_text_cords_y = hours_text_cords_y + 5 + segment_font.size_y;
        hagl_put_text(display_handle, formatted_str, CALC_STR_X_POS(clip, time_str, segment_font, 100),
        minutes_text_cords_y, text_color, segment_font.font_data);
    }
}

void clock_widget::display_seconds(tm current_time)
{
    snprintf(time_str, 64, "%02i", current_time.tm_sec);
    mbstowcs(formatted_str, time_str, 64);
    seconds_text_cords_y = minutes_text_cords_y + 5 + segment_font.size_y;
    hagl_put_text_scale(display_handle, formatted_str, CALC_STR_X_POS(clip, time_str, segment_font, 75),
    seconds_text_cords_y, 75, text_color, segment_font.font_data);
}

void clock_widget::display_date(tm current_time)
{
    if (current_time.tm_mday != reference_time.tm_mday || first_run)
    {
        current_time.tm_year += 1900;
        month_to_str(current_time.tm_mon, month_str);
        snprintf(time_str, 64, "%02i %s %04i",
        current_time.tm_mday, month_str, current_time.tm_year);
        mbstowcs(formatted_str, time_str, 64);
        months_text_cords_y = seconds_text_cords_y + 5 + segment_font.size_y;
        hagl_put_text(display_handle, formatted_str, DISPLAY_WIDTH/2 - strlen(time_str)*4, months_text_cords_y, text_color, font10x20.font_data); // Display string
    }
}

void clock_widget::display_weekday(tm current_time)
{
    if (current_time.tm_wday != reference_time.tm_wday || first_run)
    {
        weekday_to_str(current_time.tm_wday, weekday_str);
        mbstowcs(formatted_weekday, weekday_str, 10);
        weekday_text_cords_y = months_text_cords_y + 5 + font10x20.size_y;
        hagl_put_text(display_handle, formatted_weekday, DISPLAY_WIDTH/2 - strlen(weekday_str)*4, weekday_text_cords_y, text_color, font10x20.font_data); // Display string
    }
}

void clock_widget::run_widget()
{
    // text_color = hagl_color(display_handle, TEXT_COLOR);
    // hours_text_cords_y = (clip.y1-clip.y0)/10;

    while (1)
    {

        // START_DRAW(display_handle, clip, display_mutex);

        struct tm current_time;
        ds3231_get_datetime(rtc_handle, &current_time);

        display_hours(current_time);
        // display_minutes(current_time);
        // display_seconds(current_time);
        // display_date(current_time);
        // display_weekday(current_time);

        reference_time = current_time;
        if (first_run)
        {
            first_run = false;
        }

        // END_DRAW(display_mutex);

        #ifdef CHECK_TASK_SIZES
            ESP_LOGI("clock_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        END_ITERATION(1000, task_handle, task_deletion_mutex)
    }
}
