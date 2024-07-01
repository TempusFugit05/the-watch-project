
#include "clock.h"
#include "ds3231.h"
#include "config.h"

#include"esp_log.h"

#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*Pre-tested values for digits font*/
#define DIGITS_FONT_SIZE     70
#define DIGITS_CACHE_SIZE    6144

/*Struct to hold ui elements for ease of use*/
typedef struct
{
    lv_font_t* digits_font;
    lv_font_t* seconds_font;

    lv_style_t date_style;
    lv_style_t digits_style;

    lv_obj_t* hours_label;
    lv_obj_t* minutes_label;
    lv_obj_t* seconds_label;
    lv_obj_t* date_label;
    lv_obj_t* grid;
} elements;

static void month_to_str(int month, char* buffer)
{
    /*This function converts a month index intop a string and writes it to a buffer*/
    static const char* const months_of_year[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    strcpy(buffer, months_of_year[month]); // Copy the month string into the buffer
}

static void weekday_to_str(int weekday, char* buffer)
{
    /*Convert weekday into string and writes it into a buffer*/
    static const char* const days_of_week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    strcpy(buffer, days_of_week[weekday]); // Copy the day of week string into the buffer
}

static void init_fonts(elements* ui_elements)
{
    /*Init fonts for the digits*/
    extern const uint8_t digital_7_mono_font[];
    extern const int digital_7_mono_font_size;
    
    ui_elements->digits_font = lv_tiny_ttf_create_data_ex(digital_7_mono_font, digital_7_mono_font_size, DIGITS_FONT_SIZE, DIGITS_CACHE_SIZE);
    ui_elements->seconds_font = lv_tiny_ttf_create_data(digital_7_mono_font, digital_7_mono_font_size, 2*DIGITS_FONT_SIZE/3);
}

static void init_grid(elements* ui_elements)
{
    /*Init grid for labels layout*/
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(3), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};

    ui_elements->grid = lv_obj_create(lv_scr_act());
    lv_obj_set_style_grid_column_dsc_array(ui_elements->grid, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(ui_elements->grid, row_dsc, 0);
    lv_obj_set_size(ui_elements->grid, DISPLAY_SIZE_X, DISPLAY_SIZE_Y);
    lv_obj_center(ui_elements->grid);
    lv_obj_set_layout(ui_elements->grid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(ui_elements->grid, 0, 0);
    lv_obj_set_style_border_opa(ui_elements->grid, 0, 0);
}

static void init_styles(elements* ui_elements)
{
    /*Init styles*/
    lv_style_init(&ui_elements->date_style);
    lv_style_set_text_align(&ui_elements->date_style, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&ui_elements->digits_style);
    lv_style_set_text_align(&ui_elements->digits_style, LV_TEXT_ALIGN_CENTER);
}

static void init_labels(elements* ui_elements)
{
    /*Init text objects*/
    ui_elements->hours_label = lv_label_create(ui_elements->grid);
    lv_obj_add_style(ui_elements->hours_label, &ui_elements->digits_style, 0);
    lv_obj_set_grid_cell(ui_elements->hours_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    ui_elements->minutes_label = lv_label_create(ui_elements->grid);
    lv_obj_add_style(ui_elements->minutes_label, &ui_elements->digits_style, 0);
    lv_obj_set_grid_cell(ui_elements->minutes_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    ui_elements->seconds_label = lv_label_create(ui_elements->grid);
    lv_obj_add_style(ui_elements->seconds_label, &ui_elements->digits_style, 0);
    lv_obj_set_grid_cell(ui_elements->seconds_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);

    if (ui_elements->seconds_label != NULL)
    {
        lv_obj_set_style_text_font(ui_elements->seconds_label, ui_elements->seconds_font, 0);
    }

    ui_elements->date_label = lv_label_create(ui_elements->grid);
    lv_obj_add_style(ui_elements->date_label, &ui_elements->date_style, 0);
    lv_obj_set_grid_cell(ui_elements->date_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
}

static bool init_ui_elements(elements* ui_elements)
{
    /*Initialize the ui elements for the clock. Return true upon success, return false on failed initialization*/

    static const char TAG[] = "clock_ui_init";

    /*Initialize pointers to objects as NULL for error handling*/
    ui_elements->digits_font = NULL;
    ui_elements->hours_label = NULL;
    ui_elements->minutes_label = NULL;
    ui_elements->seconds_label = NULL;
    ui_elements->date_label = NULL;
    ui_elements->grid = NULL;

    init_fonts(ui_elements);
    init_grid(ui_elements);
    init_styles(ui_elements);

    /*Assign font to digits style unless font memory allocation failed*/
    if (ui_elements->digits_font != NULL)
    {
        lv_style_set_text_font(&ui_elements->digits_style, ui_elements->digits_font);
    }

    else
    {
        ESP_LOGW(TAG, "Failed to create font; falling back on default font...");
    }

    init_labels(ui_elements);

    if (ui_elements->hours_label == NULL || ui_elements->minutes_label == NULL ||
        ui_elements->seconds_label == NULL || ui_elements->date_label == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for labels!");
        return false; // Return false if object allocation failed
    }

    return true; // Return true if object allocation was successful
    
}

static void deinit_ui_elements(elements* ui_elements)
{
    /*Destroy all created ui elements*/

    /*delete fonts if they were created*/
    if (ui_elements->digits_font != NULL)
    {
        lv_tiny_ttf_destroy(ui_elements->digits_font);
    }
    
    if (ui_elements->seconds_font != NULL)
    {
        lv_tiny_ttf_destroy(ui_elements->seconds_font);
    }
    

    /*Delete styles*/
    lv_style_reset(&ui_elements->date_style);
    lv_style_reset(&ui_elements->digits_style);

    /*Delete labels*/
    lv_obj_del(ui_elements->grid);
}

static void display_time_value(const int current_time_value, const int prev_time_value, lv_obj_t* label)
{
    /*Function to update hours, minutes and seconds labels*/

    if(current_time_value != prev_time_value)
    {
        lv_label_set_text_fmt(label, "%02d", current_time_value);
    }
}

static void display_date(const struct tm* current_time, const struct tm* reference_time, elements* ui_elements)
{
    /*Function to display the month, day of month, year and weekday*/

    /*Buffers for week and month string storage*/
    static char weekday_buff[10];
    static char month_buff[10];

    if (current_time->tm_mday != reference_time->tm_mday)
    {
        month_to_str(current_time->tm_mon, month_buff);
        weekday_to_str(current_time->tm_wday, weekday_buff);
        lv_label_set_text_fmt(ui_elements->date_label, "%s %02d %d\n%s", month_buff, current_time->tm_mday, current_time->tm_year + 1900, weekday_buff);

    }
}

void run_clock_widget(void* rtc)
{
    struct tm current_time;
    struct tm reference_time = 
    {.tm_sec=-1,
     .tm_min=-1,
     .tm_hour=-1,
     .tm_mday=-1,
     .tm_mon=-1,
     .tm_year=-1,
     .tm_wday=-1}; // Init all values to -1 to ensure labels being updated upon first run

    elements ui_elements; // Struct to hold all ui elements
    if(!init_ui_elements(&ui_elements))
    {
        abort();
    } // Initialize ui elements. If this fails, crush the program

    ds3231_handle_t* rtc_handle = (ds3231_handle_t*)rtc; // Get rtc object
    
    while (1)
    {
        ds3231_get_datetime(rtc_handle, &current_time); // Get current time

        display_time_value(current_time.tm_hour, reference_time.tm_hour, ui_elements.hours_label); // Display hours
        display_time_value(current_time.tm_min, reference_time.tm_min, ui_elements.minutes_label); // Display minutes
        display_time_value(current_time.tm_sec, reference_time.tm_sec, ui_elements.seconds_label); // Display seconds
        display_date(&current_time, &reference_time, &ui_elements); // Display date

        reference_time = current_time; // Update reference time
        
        #ifdef CHECK_TASK_SIZES
            ESP_LOGI("clock_widget", "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }

    deinit_ui_elements(&ui_elements); // Destroy ui elements
}
