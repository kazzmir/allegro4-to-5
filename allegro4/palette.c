#include <allegro5/allegro.h>

#include "allegro.h"
#include "include/internal/aintern.h"

void set_palette(const PALETTE palette){
    memcpy(current_palette, palette, sizeof(PALETTE));
}

void get_palette(PALETTE palette){
    memcpy(palette, current_palette, sizeof(PALETTE));
}


void select_palette(AL_CONST PALETTE p)
{
    // TODO
}

void unselect_palette(void)
{
    // TODO
}

void generate_332_palette(PALETTE pal)
{
    // TODO
}

int generate_optimized_palette(struct BITMAP *image, PALETTE pal, AL_CONST signed char rsvdcols[256])
{
    // TODO
    return 0;
}
