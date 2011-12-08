#ifndef ALLEGRO_H
#define ALLEGRO_H

/* FIXME: not sure if stdlib belongs here */
#include <stdlib.h>

#include "include/base.h"
#include "include/gfx.h"
#include "include/draw.h"
#include "include/fix.h"
#include "include/matrix.h"
#include "include/palette.h"
#include "include/text.h"
#include "include/keyboard.h"
#include "include/joystick.h"
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
#include "include/config.h"
#include "include/rle.h"
#include "include/compiled.h"
#include "include/system.h"

#ifdef __cplusplus
   extern "C" {
#endif

// typedef struct BITMAP BITMAP;
extern int * palette_color;
extern BITMAP* screen;

#define END_OF_MAIN()

#ifdef __cplusplus
    }
#endif

#endif
