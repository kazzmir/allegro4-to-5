#include <stdio.h>
#include <stdlib.h>
#include "allegro.h"

#include "include/internal/aintern.h"

int _rgb_r_shift_15 = DEFAULT_RGB_R_SHIFT_15;     /* truecolor pixel format */
int _rgb_g_shift_15 = DEFAULT_RGB_G_SHIFT_15;
int _rgb_b_shift_15 = DEFAULT_RGB_B_SHIFT_15;
int _rgb_r_shift_16 = DEFAULT_RGB_R_SHIFT_16;
int _rgb_g_shift_16 = DEFAULT_RGB_G_SHIFT_16;
int _rgb_b_shift_16 = DEFAULT_RGB_B_SHIFT_16;
int _rgb_r_shift_24 = DEFAULT_RGB_R_SHIFT_24;
int _rgb_g_shift_24 = DEFAULT_RGB_G_SHIFT_24;
int _rgb_b_shift_24 = DEFAULT_RGB_B_SHIFT_24;
int _rgb_r_shift_32 = DEFAULT_RGB_R_SHIFT_32;
int _rgb_g_shift_32 = DEFAULT_RGB_G_SHIFT_32;
int _rgb_b_shift_32 = DEFAULT_RGB_B_SHIFT_32;
int _rgb_a_shift_32 = DEFAULT_RGB_A_SHIFT_32;

PALETTE black_palette;

PALETTE desktop_palette = 
{
   { 63, 63, 63, 0 },   { 63, 0,  0,  0 },   { 0,  63, 0,  0 },   { 63, 63, 0,  0 },
   { 0,  0,  63, 0 },   { 63, 0,  63, 0 },   { 0,  63, 63, 0 },   { 16, 16, 16, 0 },
   { 31, 31, 31, 0 },   { 63, 31, 31, 0 },   { 31, 63, 31, 0 },   { 63, 63, 31, 0 },
   { 31, 31, 63, 0 },   { 63, 31, 63, 0 },   { 31, 63, 63, 0 },   { 0,  0,  0,  0 }
};

PALETTE default_palette =
{
   { 0,  0,  0,  0 },   { 0,  0,  42, 0 },   { 0,  42, 0,  0 },   { 0,  42, 42, 0 },
   { 42, 0,  0,  0 },   { 42, 0,  42, 0 },   { 42, 21, 0,  0 },   { 42, 42, 42, 0 },
   { 21, 21, 21, 0 },   { 21, 21, 63, 0 },   { 21, 63, 21, 0 },   { 21, 63, 63, 0 },
   { 63, 21, 21, 0 },   { 63, 21, 63, 0 },   { 63, 63, 21, 0 },   { 63, 63, 63, 0 },
   { 0,  0,  0,  0 },   { 5,  5,  5,  0 },   { 8,  8,  8,  0 },   { 11, 11, 11, 0 },
   { 14, 14, 14, 0 },   { 17, 17, 17, 0 },   { 20, 20, 20, 0 },   { 24, 24, 24, 0 },
   { 28, 28, 28, 0 },   { 32, 32, 32, 0 },   { 36, 36, 36, 0 },   { 40, 40, 40, 0 },
   { 45, 45, 45, 0 },   { 50, 50, 50, 0 },   { 56, 56, 56, 0 },   { 63, 63, 63, 0 },
   { 0,  0,  63, 0 },   { 16, 0,  63, 0 },   { 31, 0,  63, 0 },   { 47, 0,  63, 0 },
   { 63, 0,  63, 0 },   { 63, 0,  47, 0 },   { 63, 0,  31, 0 },   { 63, 0,  16, 0 },
   { 63, 0,  0,  0 },   { 63, 16, 0,  0 },   { 63, 31, 0,  0 },   { 63, 47, 0,  0 },
   { 63, 63, 0,  0 },   { 47, 63, 0,  0 },   { 31, 63, 0,  0 },   { 16, 63, 0,  0 },
   { 0,  63, 0,  0 },   { 0,  63, 16, 0 },   { 0,  63, 31, 0 },   { 0,  63, 47, 0 },
   { 0,  63, 63, 0 },   { 0,  47, 63, 0 },   { 0,  31, 63, 0 },   { 0,  16, 63, 0 },
   { 31, 31, 63, 0 },   { 39, 31, 63, 0 },   { 47, 31, 63, 0 },   { 55, 31, 63, 0 },
   { 63, 31, 63, 0 },   { 63, 31, 55, 0 },   { 63, 31, 47, 0 },   { 63, 31, 39, 0 },
   { 63, 31, 31, 0 },   { 63, 39, 31, 0 },   { 63, 47, 31, 0 },   { 63, 55, 31, 0 },
   { 63, 63, 31, 0 },   { 55, 63, 31, 0 },   { 47, 63, 31, 0 },   { 39, 63, 31, 0 },
   { 31, 63, 31, 0 },   { 31, 63, 39, 0 },   { 31, 63, 47, 0 },   { 31, 63, 55, 0 },
   { 31, 63, 63, 0 },   { 31, 55, 63, 0 },   { 31, 47, 63, 0 },   { 31, 39, 63, 0 },
   { 45, 45, 63, 0 },   { 49, 45, 63, 0 },   { 54, 45, 63, 0 },   { 58, 45, 63, 0 },
   { 63, 45, 63, 0 },   { 63, 45, 58, 0 },   { 63, 45, 54, 0 },   { 63, 45, 49, 0 },
   { 63, 45, 45, 0 },   { 63, 49, 45, 0 },   { 63, 54, 45, 0 },   { 63, 58, 45, 0 },
   { 63, 63, 45, 0 },   { 58, 63, 45, 0 },   { 54, 63, 45, 0 },   { 49, 63, 45, 0 },
   { 45, 63, 45, 0 },   { 45, 63, 49, 0 },   { 45, 63, 54, 0 },   { 45, 63, 58, 0 },
   { 45, 63, 63, 0 },   { 45, 58, 63, 0 },   { 45, 54, 63, 0 },   { 45, 49, 63, 0 },
   { 0,  0,  28, 0 },   { 7,  0,  28, 0 },   { 14, 0,  28, 0 },   { 21, 0,  28, 0 },
   { 28, 0,  28, 0 },   { 28, 0,  21, 0 },   { 28, 0,  14, 0 },   { 28, 0,  7,  0 },
   { 28, 0,  0,  0 },   { 28, 7,  0,  0 },   { 28, 14, 0,  0 },   { 28, 21, 0,  0 },
   { 28, 28, 0,  0 },   { 21, 28, 0,  0 },   { 14, 28, 0,  0 },   { 7,  28, 0,  0 },
   { 0,  28, 0,  0 },   { 0,  28, 7,  0 },   { 0,  28, 14, 0 },   { 0,  28, 21, 0 },
   { 0,  28, 28, 0 },   { 0,  21, 28, 0 },   { 0,  14, 28, 0 },   { 0,  7,  28, 0 },
   { 14, 14, 28, 0 },   { 17, 14, 28, 0 },   { 21, 14, 28, 0 },   { 24, 14, 28, 0 },
   { 28, 14, 28, 0 },   { 28, 14, 24, 0 },   { 28, 14, 21, 0 },   { 28, 14, 17, 0 },
   { 28, 14, 14, 0 },   { 28, 17, 14, 0 },   { 28, 21, 14, 0 },   { 28, 24, 14, 0 },
   { 28, 28, 14, 0 },   { 24, 28, 14, 0 },   { 21, 28, 14, 0 },   { 17, 28, 14, 0 },
   { 14, 28, 14, 0 },   { 14, 28, 17, 0 },   { 14, 28, 21, 0 },   { 14, 28, 24, 0 },
   { 14, 28, 28, 0 },   { 14, 24, 28, 0 },   { 14, 21, 28, 0 },   { 14, 17, 28, 0 },
   { 20, 20, 28, 0 },   { 22, 20, 28, 0 },   { 24, 20, 28, 0 },   { 26, 20, 28, 0 },
   { 28, 20, 28, 0 },   { 28, 20, 26, 0 },   { 28, 20, 24, 0 },   { 28, 20, 22, 0 },
   { 28, 20, 20, 0 },   { 28, 22, 20, 0 },   { 28, 24, 20, 0 },   { 28, 26, 20, 0 },
   { 28, 28, 20, 0 },   { 26, 28, 20, 0 },   { 24, 28, 20, 0 },   { 22, 28, 20, 0 },
   { 20, 28, 20, 0 },   { 20, 28, 22, 0 },   { 20, 28, 24, 0 },   { 20, 28, 26, 0 },
   { 20, 28, 28, 0 },   { 20, 26, 28, 0 },   { 20, 24, 28, 0 },   { 20, 22, 28, 0 },
   { 0,  0,  16, 0 },   { 4,  0,  16, 0 },   { 8,  0,  16, 0 },   { 12, 0,  16, 0 },
   { 16, 0,  16, 0 },   { 16, 0,  12, 0 },   { 16, 0,  8,  0 },   { 16, 0,  4,  0 },
   { 16, 0,  0,  0 },   { 16, 4,  0,  0 },   { 16, 8,  0,  0 },   { 16, 12, 0,  0 },
   { 16, 16, 0,  0 },   { 12, 16, 0,  0 },   { 8,  16, 0,  0 },   { 4,  16, 0,  0 },
   { 0,  16, 0,  0 },   { 0,  16, 4,  0 },   { 0,  16, 8,  0 },   { 0,  16, 12, 0 },
   { 0,  16, 16, 0 },   { 0,  12, 16, 0 },   { 0,  8,  16, 0 },   { 0,  4,  16, 0 },
   { 8,  8,  16, 0 },   { 10, 8,  16, 0 },   { 12, 8,  16, 0 },   { 14, 8,  16, 0 },
   { 16, 8,  16, 0 },   { 16, 8,  14, 0 },   { 16, 8,  12, 0 },   { 16, 8,  10, 0 },
   { 16, 8,  8,  0 },   { 16, 10, 8,  0 },   { 16, 12, 8,  0 },   { 16, 14, 8,  0 },
   { 16, 16, 8,  0 },   { 14, 16, 8,  0 },   { 12, 16, 8,  0 },   { 10, 16, 8,  0 },
   { 8,  16, 8,  0 },   { 8,  16, 10, 0 },   { 8,  16, 12, 0 },   { 8,  16, 14, 0 },
   { 8,  16, 16, 0 },   { 8,  14, 16, 0 },   { 8,  12, 16, 0 },   { 8,  10, 16, 0 },
   { 11, 11, 16, 0 },   { 12, 11, 16, 0 },   { 13, 11, 16, 0 },   { 15, 11, 16, 0 },
   { 16, 11, 16, 0 },   { 16, 11, 15, 0 },   { 16, 11, 13, 0 },   { 16, 11, 12, 0 },
   { 16, 11, 11, 0 },   { 16, 12, 11, 0 },   { 16, 13, 11, 0 },   { 16, 15, 11, 0 },
   { 16, 16, 11, 0 },   { 15, 16, 11, 0 },   { 13, 16, 11, 0 },   { 12, 16, 11, 0 },
   { 11, 16, 11, 0 },   { 11, 16, 12, 0 },   { 11, 16, 13, 0 },   { 11, 16, 15, 0 },
   { 11, 16, 16, 0 },   { 11, 15, 16, 0 },   { 11, 13, 16, 0 },   { 11, 12, 16, 0 },
   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },
   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 63, 63, 63, 0 }
};


/* 1.5k lookup table for color matching */
static unsigned int col_diff[3*128]; 

/* bestfit_init:
 *  Color matching is done with weighted squares, which are much faster
 *  if we pregenerate a little lookup table...
 */
static void bestfit_init(void)
{
   int i;

   for (i=1; i<64; i++) {
      int k = i * i;
      col_diff[0  +i] = col_diff[0  +128-i] = k * (59 * 59);
      col_diff[128+i] = col_diff[128+128-i] = k * (30 * 30);
      col_diff[256+i] = col_diff[256+128-i] = k * (11 * 11);
   }
}

/* bestfit_color:
 *  Searches a palette for the color closest to the requested R, G, B value.
 */
int bestfit_color(AL_CONST PALETTE pal, int r, int g, int b)
{
   int i, coldiff, lowest, bestfit;

   ASSERT(r >= 0 && r <= 63);
   ASSERT(g >= 0 && g <= 63);
   ASSERT(b >= 0 && b <= 63);

   if (col_diff[1] == 0)
      bestfit_init();

   bestfit = 0;
   lowest = INT_MAX;

   /* only the transparent (pink) color can be mapped to index 0 */
   if ((r == 63) && (g == 0) && (b == 63))
      i = 0;
   else
      i = 1;

   while (i<PAL_SIZE) {
      AL_CONST RGB *rgb = &pal[i];
      coldiff = (col_diff + 0) [ (rgb->g - g) & 0x7F ];
      if (coldiff < lowest) {
         coldiff += (col_diff + 128) [ (rgb->r - r) & 0x7F ];
         if (coldiff < lowest) {
            coldiff += (col_diff + 256) [ (rgb->b - b) & 0x7F ];
            if (coldiff < lowest) {
               bestfit = rgb - pal;    /* faster than `bestfit = i;' */
               if (coldiff == 0)
                  return bestfit;
               lowest = coldiff;
            }
         }
      }
      i++;
   }

   return bestfit;
}

int makecol8(int r, int g, int b)
{
    return bestfit_color(current_palette, r>>2, g>>2, b>>2);
}

/* makecol_depth:
 *  Converts R, G, and B values (ranging 0-255) to whatever pixel format
 *  is required by the specified color depth.
 */
int makecol_depth(int color_depth, int r, int g, int b)
{
    switch (color_depth) {

    case 8:
        return makecol8(r, g, b);

    case 15:
        return makecol15(r, g, b);

    case 16:
        return makecol16(r, g, b);

    case 24:
        return makecol24(r, g, b);

    case 32:
        return makecol32(r, g, b);
    }

    return 0;
}

int makeacol_depth(int color_depth, int r, int g, int b, int a)
{
   switch (color_depth) {

      case 8:
	 return makecol8(r, g, b);

      case 15:
	 return makecol15(r, g, b);

      case 16:
	 return makecol16(r, g, b);

      case 24:
	 return makecol24(r, g, b);

      case 32:
	 return makeacol32(r, g, b, a);
   }

   return 0;
}

void set_color(int idx, AL_CONST RGB *p){
    /* TODO */
}

void get_color(int idx, RGB *p){
    /* TODO */
}

void fade_in(AL_CONST PALETTE p, int speed){
    /* TODO */
}

void fade_out(int speed){
    ALLEGRO_STATE state;
    ALLEGRO_BITMAP *scr;
    ALLEGRO_BITMAP *copy;
    ALLEGRO_TRANSFORM t;
    int c;

    scr = screen->real;
    copy = al_clone_bitmap(scr);

    al_store_state(&state,
	ALLEGRO_STATE_TARGET_BITMAP | ALLEGRO_STATE_BLENDER |
	ALLEGRO_STATE_TRANSFORM);
    al_set_target_bitmap(scr);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
    al_identity_transform(&t);
    al_use_transform(&t);

    for (c=255; c>=0; c-=4*speed) {
        int i = MID(0, c, 255);
        /* twice */
        al_draw_tinted_bitmap(copy, al_map_rgb(i, i, i), 0, 0, 0);
        al_flip_display();
        al_draw_tinted_bitmap(copy, al_map_rgb(i, i, i), 0, 0, 0);
        al_flip_display();
    }

    al_restore_state(&state);

    al_destroy_bitmap(copy);
}
