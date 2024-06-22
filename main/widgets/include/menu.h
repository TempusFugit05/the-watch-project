#ifndef MENU_H
#define MENU_H
#include "widget.h"
#include "fonts.h"

class button
{
private:
    char* text; // Text to be desplayed
    font font;    
    void (*action); // Function pointer to button action

    hagl_color_t bg_color;
    hagl_color_t text_color;

    coordinates position;
    uint16_t size_x;
    uint16_t size_y;

    uint8_t index;

public:
    button(/* args */);
    ~button();
};

button::button(/* args */)
{
}

button::~button()
{
}

class menu : protected widget
{
private:
    button* buttons;

    uint8_t num_buttons;
    uint8_t populated_spots = 0;

public:
    void add_button(button button_to_add);
    menu(/* args */);
    ~menu();
};

menu::menu(hagl_backend_t* display, SemaphoreHandle_t display_mutex, hagl_window_t clip, uint8_t num_buttons)
: widget(display, display_mutex, clip, "menu_widget"), num_buttons(num_buttons)
{
    buttons = pvPortMalloc(sizeof(button) * num_buttons); // Allocate memory for buttons inside the menu
}

menu::~menu()
{
    vPortFree(buttons);
}

void menu::add_button(button* button_to_add)
{
    if (populated_spots < num_buttons)
    {
        buttons[populated_spots] = button_to_add;
        populated_spots ++;
    } // Populate button spot inside the array

    else
    {
        ESP_LOGE(WIDGET_NAME, "All button spots are populated!");
    }
    
}


#endif // MENU_H