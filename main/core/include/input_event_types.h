#ifndef INPUT_EVENT_TYPES_H
#define INPUT_EVENT_TYPES_H

typedef enum : uint8_t{
    INIT_EVENT,
    IDLE_EVENT,
    SHORT_PRESS_EVENT,
    LONG_PRESS_EVENT,
    SCROLL_UP_EVENT,
    SCROLL_DOWN_EVENT,
}input_event_t; // Input event flags for the input events queue

#endif // INPUT_EVENT_TYPES_H