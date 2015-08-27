#ifndef ALLEGRO_H
#define ALLEGRO_H

/* FIXME: not sure if stdlib belongs here */
#include <stdlib.h>

#define BITMAP WIN32BITMAP
#include <allegro5/allegro.h>

#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_windows.h>
#endif

#undef BITMAP

#undef AL_FUNC
#undef AL_VAR
#undef AL_FUNCPTR
#undef AL_ARRAY

#include "include/base.h"
#include "include/gfx.h"
#include "include/draw.h"
#include "include/matrix.h"
#include "include/3dmaths.h"
#include "include/fix.h"
#include "include/palette.h"
#include "include/text.h"
#include "include/keyboard.h"
#include "include/joystick.h"
#include "include/mouse.h"
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
#include "include/config.h"
#include "include/rle.h"
#include "include/compiled.h"
#include "include/system.h"
#include "include/timer.h"
#include "include/quat.h"
#include "include/extensions.h"

#ifdef __cplusplus
   extern "C" {
#endif

#undef MIN
#undef MAX
#undef MID
     
#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))

/* Returns the median of x, y, z */
#define MID(x,y,z)   ((x) > (y) ? ((y) > (z) ? (y) : ((x) > (z) ?    \
                       (z) : (x))) : ((y) > (z) ? ((z) > (x) ? (z) : \
                       (x)): (y)))

/* Optimized version of MID for when x <= z. */
#define CLAMP(x,y,z) MAX((x), MIN((y), (z)))
     
#undef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))
     
#undef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)

#define _color_depth current_depth

#define _mouse_screen screen

// typedef struct BITMAP BITMAP;
extern int * palette_color;
extern BITMAP* screen;
extern PALETTE current_palette;
extern int current_depth;

unsigned int _default_ds();

extern int AL_RAND();

#define bmp_read8(addr)             (*((uint8_t  *)(addr)))
#define bmp_write8(addr, c)         (*((uint8_t  *)(addr)) = (c))

#define bmp_read32(addr)            (*((uint32_t *)(addr)))
#define bmp_write32(addr, c)        (*((uint32_t *)(addr)) = (c))

#define OLD_FILESEL_WIDTH   -1
#define OLD_FILESEL_HEIGHT  -1

#define END_OF_MAIN()

#ifdef __cplusplus
    }
#endif

#endif
