
#include "../ds3231/ds3231.h"
#include"esp_log.h"
#include "lvgl.h"

void month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer*/
    static const char* months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    strcpy(buffer, months_of_year[month]); // Copy the month string into the buffer
}

void weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer*/
    static const char* days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday]); // Copy the day of week string into the buffer
}

void display_hours(tm current_time, lv_obj_t* label)
{
    lv_label_set_text_fmt(label, "%02d", current_time.tm_hour);
}

void display_minutes(tm current_time, lv_obj_t* label)
{
    lv_label_set_text_fmt(label, "%02d", current_time.tm_sec);
}

void run_widget(void* rtc)
{
    struct tm current_time;
    
    ds3231_handle_t* rtc_handle = (ds3231_handle_t*)rtc;

    ds3231_get_datetime(rtc_handle, &current_time);

    extern const uint8_t ds_digi[];
    extern const int ds_digi_size;

    lv_font_t* font = lv_tiny_ttf_create_data(ds_digi, ds_digi_size, 48);
    
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&style, LV_ALIGN_TOP_MID);

    static lv_style_t digits_style;
    lv_style_init(&digits_style);
    lv_style_set_text_font(&digits_style, font);
    lv_style_set_text_align(&digits_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&digits_style, LV_ALIGN_TOP_MID);


    lv_obj_t* hours_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(hours_label, &digits_style, 0);
    lv_obj_set_y(hours_label, lv_pct(100/6));

    lv_obj_t* minutes_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(minutes_label, &digits_style, 0);
    lv_obj_set_y(minutes_label, lv_pct(200/6));

    lv_obj_t* seconds_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(seconds_label, &digits_style, 0);
    lv_obj_set_y(seconds_label, lv_pct(300/6));

    lv_obj_t* date_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(date_label, &style, 0);
    lv_obj_set_y(date_label, lv_pct(400/6));

    lv_obj_t* weekday_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(weekday_label, &style, 0);
    lv_obj_set_y(weekday_label, lv_pct(500/6));

    char weekday_buff[10];
    char month_buff[10];

    while (1)
    {
        ds3231_get_datetime(rtc_handle, &current_time);

        lv_label_set_text_fmt(hours_label, "%02d:", current_time.tm_hour);

        lv_label_set_text_fmt(minutes_label, "%02d:", current_time.tm_min);

        lv_label_set_text_fmt(seconds_label, "%02d:", current_time.tm_sec);

        month_to_str(current_time.tm_mon, month_buff);
        weekday_to_str(current_time.tm_wday, weekday_buff);
        lv_label_set_text_fmt(date_label, "%s %02d %d\n%s", month_buff, current_time.tm_mday, current_time.tm_year + 1900, weekday_buff);

        // lv_label_set_text_fmt(weekday_label, "%s", weekday_buff);

        #ifdef CHECK_TASK_SIZES
            ESP_LOGI("clock_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
