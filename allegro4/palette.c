#include "allegro.h"
#include "include/internal/aintern.h"

#include <allegro5/allegro.h>

void set_palette(const PALETTE palette){
    set_palette_range(palette, 0, 255, FALSE);
}

void set_palette_range(AL_CONST PALETTE p, int from, int to, int retracesync)
{
    int i;
    if (retracesync) {
        al_wait_for_vsync();
    }
    for (i = from; i <= to; i++) {
        current_palette[i] = p[i];
    }
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
