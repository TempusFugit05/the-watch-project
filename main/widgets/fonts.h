#ifndef FONTS_H
#define FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int size_x;
    int size_y;
    const unsigned char* font_data;
} font;

extern font segment_font;
extern font font10x20;
extern font font9x18b;
extern font font6x9;

#ifdef __cplusplus
}
#endif

#endif // FONTS_H