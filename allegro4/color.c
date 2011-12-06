#include <stdio.h>
#include <stdlib.h>
#include "allegro.h"

PALETTE desktop_palette = 
{
   { 63, 63, 63, 0 },   { 63, 0,  0,  0 },   { 0,  63, 0,  0 },   { 63, 63, 0,  0 },
   { 0,  0,  63, 0 },   { 63, 0,  63, 0 },   { 0,  63, 63, 0 },   { 16, 16, 16, 0 },
   { 31, 31, 31, 0 },   { 63, 31, 31, 0 },   { 31, 63, 31, 0 },   { 63, 63, 31, 0 },
   { 31, 31, 63, 0 },   { 63, 31, 63, 0 },   { 31, 63, 63, 0 },   { 0,  0,  0,  0 }
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
