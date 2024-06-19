#ifndef WIDGET_UTILS_H
#define WIDGET_UTILS_H

// Set drawable area
#define SET_CLIP(display, clip) hagl_set_clip(display, clip.x0, clip.y0, clip.x1, clip.y1);

// Start drawing on display and block other tasks from drawing
#define START_DRAW(display, clip, mutex)\ 
    if (xSemaphoreTake(display_mutex, pdMS_TO_TICKS(portMAX_DELAY)) == pdTRUE) \
        {SET_CLIP(display_handle,clip); // Set drawable area

// End drawing sequence, release display for others
#define END_DRAW(mutex) xSemaphoreGive(mutex);}

// Fill drawable area with black pixles
#define CLEAR_SCREEN(display, clip, mutex) \
    START_DRAW(display, clip, mutex) \
    hagl_fill_rectangle(display, clip.x0, clip.y0, clip.x1, clip.y1, hagl_color(display, 0, 0, 0)); \
    END_DRAW(mutex) \

#define TASK_DELETION_CODE 42 // The value of the task notification for deletion notification

// Send notification to task to delete itself
#define START_WIDGET_DELETION(task_mutex) \
    xSemaphoreGive(task_mutex); \
                ESP_LOGI("deleted task","");\
    if (xSemaphoreTake(task_mutex, portMAX_DELAY) != pdTRUE) \
    { \

// End deletion sequence
#define END_WIDGET_DELETION(task_mutex) \
        vSemaphoreDelete(task_mutex); \
    } \

// Check if task needs to be deleted
#define CHECK_WIDGET_DELETION_STATUS(task_mutex) \
    int buf; \
    xQueuePeek(task_mutex, &buf, 0); \
    if (buf == pdTRUE) \
        { \
            ESP_LOGI("deleted task","");\
        } \
          // xSemaphoreGive(task_mutex); 
            // ESP_LOGI("deleted task","");
            // vTaskDelete(task_handle); 
#endif // WIDGET_UTILS_H