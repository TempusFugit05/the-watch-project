#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include "widgets.h"
#include "esp_timer.h"

#define SWITCHING_DELAY 200000

typedef enum : uint8_t{
    SCREEN_FACES_PADDING_LOWER,
    SCREEN_CLOCK_FACE,
    SCREEN_HEART_RATE_FACE,
    SCREEN_FACES_PADDING_UPPER,
}main_screen_faces_t;

static widget *widget_instance = NULL;    

bool input_event_redirector(input_event_t event)
{
    /*This function is responsible for redirecting the input events to the relevant widgets*/
    if (widget_instance != NULL)
    {
        if (widget_instance->get_input_requirements())
        {
            widget_instance->set_input_event(event);
            return true; // True if input is redirected
        }
    } // Check if the active widget requires inputs

    return false; // False if input is not redirected
}

main_screen_faces_t scroll_face(input_event_t event, main_screen_faces_t current_face)
{
    switch (event)
    {
    case SCROLL_UP_EVENT:
        current_face = static_cast<main_screen_faces_t>(current_face + static_cast <main_screen_faces_t>(1));
        if (current_face >= SCREEN_FACES_PADDING_UPPER)
        {
            current_face = static_cast<main_screen_faces_t>(SCREEN_FACES_PADDING_LOWER + 1);
        } // Switch to the next widget upon scrolling the encoder CCW
        break;
    
    case SCROLL_DOWN_EVENT:
        current_face = static_cast<main_screen_faces_t>(current_face - static_cast <main_screen_faces_t>(1));
        if (current_face <= SCREEN_FACES_PADDING_LOWER)
        {
            current_face = static_cast<main_screen_faces_t>(SCREEN_FACES_PADDING_UPPER - 1);
        } // Switch to the previous widget upon scrolling the encoder CC
        break;

    default:
        break;
    }

    return current_face;
}

void main_screen_state_machine(input_event_t event)
{
    /*This function handles the switching of ui elements upon input events*/

    const int INFO_BAR_WIDTH = DISPLAY_WIDTH;
    const int INFO_BAR_HEIGHT = 20;

    hagl_window_t info_bar_clip = {.x0=0, .y0=0, .x1=INFO_BAR_WIDTH, .y1=INFO_BAR_HEIGHT};
    hagl_window_t widget_clip = {.x0=0, .y0=INFO_BAR_HEIGHT, .x1=DISPLAY_WIDTH, .y1=DISPLAY_HEIGHT};
    
    /*Currently running ui elements*/
    static main_screen_faces_t current_face = SCREEN_FACES_PADDING_LOWER;
    static info_bar_widget *info_bar = new info_bar_widget(display, display_mutex, info_bar_clip, &ds3231_dev_handle);

    static uint64_t prev_switching_time = esp_timer_get_time();

    if (event == INIT_EVENT)
    {
        widget_instance = new test_widget(display, display_mutex, widget_clip);
        // widget_instance = new snake_game_widget(display, display_mutex, widget_clip);
        current_face = SCREEN_HEART_RATE_FACE;
    } // Initialize the clock widget upon startup

    current_face = scroll_face(event, current_face); // Iterate face
    

    if (event == SCROLL_UP_EVENT || event == SCROLL_DOWN_EVENT)
    {
        if (esp_timer_get_time() - prev_switching_time >= SWITCHING_DELAY)
        {
            if (widget_instance != NULL)
            {
                delete widget_instance;
                // widget_instance = NULL;
            }
            
            switch (current_face)
            {
                case SCREEN_CLOCK_FACE:
                    {
                        widget_instance = new clock_widget(display, display_mutex, widget_clip, &ds3231_dev_handle);

                        // if (info_bar != NULL)
                        // {
                        //     delete info_bar;
                        //     info_bar = NULL;
                        // }
                        break;
                    } // Create clock widget and delete info bar widget if it exists

                case SCREEN_HEART_RATE_FACE:
                {
                    widget_instance = new test_widget(display, display_mutex, widget_clip);
                    
                    if (info_bar == NULL)
                    {
                        info_bar = new info_bar_widget(display, display_mutex, info_bar_clip, &ds3231_dev_handle);
                    }
                    break;
                }
                default:
                    break;
            } // Create widget based on the current screen face
        }
    }
}


void input_events_handler_task(void* arg)
{
    /*This tasks is responisble for handling the input events from buttons and encoders.
    It gets the info from a queue to which the input ISR's handlers send a message when they are activated*/
    
    input_event_t event; // Event type

    while(1){
        if(xQueueReceive(input_event_queue, &event, portMAX_DELAY)) // Wait for an event in the queue
        {   
            if (widget_instance != NULL)
            {
                if (widget_instance->get_input_requirements())
                {
                    input_event_redirector(event);
                } // Redirect inputs to active widget if it requires inputs

                else
                {
                    main_screen_state_machine(event);            
                }
            }

            else
            {
                main_screen_state_machine(event);            
            }
        }

        #ifdef CHECK_TASK_SIZES
            ESP_LOGI(TAG, "Task size: %i", uxTaskGetStackHighWaterMark(NULL));
        #endif
        
    }
}

#endif // WIDGET_MANAGER_H