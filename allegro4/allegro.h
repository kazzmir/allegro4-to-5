#ifndef ALLEGRO_H
#define ALLEGRO_H

#include "include/gfx.h"
#include "include/draw.h"
#include "include/fix.h"
#include "include/matrix.h"
#include "include/palette.h"
#include "include/text.h"
#include "include/keyboard.h"
#include "include/mouse.h"
#include "include/3dmaths.h"
#include "include/3d.h"
#include "include/color.h"
#include "include/unicode.h"
#include "include/file.h"
#include "include/datafile.h"
#include "include/lzss.h"
#include "include/gui.h"
#include "include/font.h"
#include "include/midi.h"
#include "include/sound.h"

#ifdef __cplusplus
   extern "C" {
#endif

// typedef struct BITMAP BITMAP;
extern int * palette_color;
extern BITMAP* screen;

extern int allegro_error;

int allegro_init();
void allegro_message(char const *format, ...);

#define END_OF_MAIN()

#ifdef __cplusplus
    }
#endif

#endif
