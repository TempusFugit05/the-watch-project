#ifndef WIDGET_UTILS_H
#define WIDGET_UTILS_H

#define SET_CLIP(display, clip) hagl_set_clip(display, clip.x0, clip.y0, clip.x1, clip.y1);
#define START_DRAW(display, clip, mutex) if (xSemaphoreTake(display_mutex, pdMS_TO_TICKS(portMAX_DELAY))){SET_CLIP(display_handle,clip); // Set drawable area
#define END_DRAW(mutex) xSemaphoreGive(mutex);}

#endif // WIDGET_UTILS_H