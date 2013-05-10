#ifndef ALLEGRO_EXTENSIONS_H
#define ALLEGRO_EXTENSIONS_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef enum
{
    CONVERT_8BIT_PALETTE,
    CONVERT_8BIT_PALETTE_REPLACE_INDEX0,
    CONVERT_8BIT_INTENSITY,
    CONVERT_8BIT_MONO_FONT
} CONVERT_8BIT;

void convert_8bit(BITMAP *bitmap, CONVERT_8BIT mode, ALLEGRO_COLOR c);

void hold_screen_refresh(int hold);

#ifdef __cplusplus
   }
#endif

#endif
