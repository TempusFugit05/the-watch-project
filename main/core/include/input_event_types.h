#ifndef INPUT_EVENT_TYPES_H
#define INPUT_EVENT_TYPES_H

typedef enum{
    INIT_EVENT,
    IDLE_EVENT,
    SHORT_PRESS_EVENT,
    LONG_PRESS_EVENT,
    SCROLL_LEFT_EVENT,
    SCROLL_RIGHT_EVENT,
}input_event_t; // Input event flags for the input events queue

#endif // INPUT_EVENT_TYPES_H
