#ifndef WIDGET_UTILS_H
#define WIDGET_UTILS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TASK_DELETION_TIMEOUT_MS pdMS_TO_TICKS(5000)
#define DISPLAY_TIMEOUT_MS  pdMS_TO_TICKS(5000)

// #define DEBUG_MUTEXES

// Set drawable area
#define SET_CLIP(display, clip) hagl_set_clip(display, clip.x0, clip.y0, clip.x1, clip.y1);

#ifdef DEBUG_MUTEXES

// Start drawing on display and block other tasks from drawing
#define START_DRAW(display, clip, mutex) \
    if (xSemaphoreTake(mutex, DISPLAY_TIMEOUT_MS) == pdTRUE) \
    { \
        TaskStatus_t task_status; \
        vTaskGetInfo(xSemaphoreGetMutexHolder(mutex), &task_status, pdFALSE, eInvalid); \
        ESP_LOGI("MUTEX DEBUGGING ", "%s has taken the mutex", task_status.pcTaskName); \
        SET_CLIP(display, clip) \
    } \
    else \
    { \
        ESP_LOGE("Display mutex", "Could not take display mutex!"); \
        abort(); \
    }

// End drawing sequence, release display for others
#define END_DRAW(mutex) \
    TaskStatus_t task_status; \
    vTaskGetInfo(xSemaphoreGetMutexHolder(mutex), &task_status, pdFALSE, eInvalid); \
    ESP_LOGI("MUTEX DEBUGGING ", "%s is releasing the mutex\n", task_status.pcTaskName); \
    xSemaphoreGive(mutex);
#endif

// Start drawing on display and block other tasks from drawing
#ifndef DEBUG_MUTEXES
#define START_DRAW(display, clip, mutex) \
    if (xSemaphoreTake(mutex, DISPLAY_TIMEOUT_MS) == pdTRUE) \
    {\
        SET_CLIP(display, clip)\
    }\
    else\
    {\
        ESP_LOGE("Display mutex", "Could not take display mutex!");\
        abort();\
    }

#define END_DRAW(mutex) xSemaphoreGive(mutex)
#endif



// Fill drawable area with black pixles
#define CLEAR_SCREEN(display, clip, mutex) \
    START_DRAW(display, clip, mutex); \
    hagl_fill_rectangle(display, clip.x0, clip.y0, clip.x1, clip.y1, hagl_color(display, 0, 0, 0)); \
    END_DRAW(mutex) \

#define TASK_DELETION_CODE 42 // The value of the task notification for deletion notification

// Send notification to task to delete itself
#define START_WIDGET_DELETION(task, deletion_mutex) \
    xTaskNotify(task, TASK_DELETION_CODE, eSetValueWithOverwrite); \
    if (xSemaphoreTake(deletion_mutex, TASK_DELETION_TIMEOUT_MS) == pdTRUE) \
    { \
        vSemaphoreDelete(deletion_mutex); \
    } \
    else \
    { \
        ESP_LOGE("Widget deletion", "Did not receive deletion semaphore from task!"); \
        abort(); \
    } \
    
    

// Check if task needs to be deleted
#define END_ITERATION(delay_ms, task, deletion_mutex) \
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(delay_ms)) == TASK_DELETION_CODE) \
        { \
            xSemaphoreGive(deletion_mutex); \
            vTaskDelete(task); \
        } \

#endif // WIDGET_UTILS_H