#include <stdio.h>
#include <stdlib.h>

#include "allegro.h"
#include "include/internal/aintern.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>

ALLEGRO_DEBUG_CHANNEL("a425")

/* color_get_font_ranges:
 *  (color vtable entry)
 *  Returns the number of character ranges in a font, or -1 if that information
 *   is not available.
 */
int get_font_ranges(FONT *f)
{
    FONT_COLOR_DATA* cf = 0;
    int ranges = 0;

    if (!f) 
        return -1;

    cf = (FONT_COLOR_DATA*)(f->data);

    while(cf) {
        FONT_COLOR_DATA* next = cf->next;
        
        ranges++;
        if (!next)
            return ranges;
         cf = next;
    }

    return -1;
}



/* color_get_font_range_begin:
 *  (color vtable entry)
 *  Get first character for font.
 */
static int color_get_font_range_begin(FONT* f, int range)
{
   FONT_COLOR_DATA* cf = 0;
   int n;

   if (!f || !f->data) 
      return -1;
      
   if (range < 0)
      range = 0;
   n = 0;

   cf = (FONT_COLOR_DATA*)(f->data);
   while(cf && n<=range) {
      FONT_COLOR_DATA* next = cf->next;
        
      if (!next || range == n)
         return cf->begin;
      cf = next;
      n++;
   }

   return -1;
}



/* color_get_font_range_end:
 *  (color vtable entry)
 *  Get last character for font range.
 */
static int color_get_font_range_end(FONT* f, int range)
{
   FONT_COLOR_DATA* cf = 0;
   int n;

   if (!f) 
      return -1;

   n = 0;

   cf = (FONT_COLOR_DATA*)(f->data);

   while(cf && (n<=range || range==-1)) {
      FONT_COLOR_DATA* next = cf->next;
      if (!next || range == n)
         return cf->end - 1;
      cf = next;
      n++;
   }

   return -1;
}



/* upgrade_to_color, upgrade_to_color_data:
 *  Helper functions. Upgrades a monochrome font to a color font.
 */
static FONT_COLOR_DATA* upgrade_to_color_data(FONT_MONO_DATA* mf)
{
    FONT_COLOR_DATA* cf = _AL_MALLOC(sizeof *cf);
    BITMAP** bits = _AL_MALLOC((mf->end - mf->begin)*sizeof *bits);
    int i;

    cf->begin = mf->begin;
    cf->end = mf->end;
    cf->bitmaps = bits;
    cf->next = 0;

    for(i = mf->begin; i < mf->end; i++) {
        FONT_GLYPH* g = mf->glyphs[i - mf->begin];
        BITMAP* b = create_bitmap_ex(8, g->w, g->h);
        //b->vtable->draw_glyph(b, g, 0, 0, 1, 0);
        int x, y;
        int p = 0;
        for (y = 0; y < g->h; y++){
            int bit = 0x80;
            for (x = 0; x < g->w; x++){
                *((unsigned char *)b->line[y] + x) = (g->dat[p] & bit) ? 1 : 0;
                bit >>= 1;
                if (bit == 0) {
                    bit = 0x80;
                    p++;
                }
            }
            if (bit != 0x80)
                p++;
        }

        bits[i - mf->begin] = b;
    }

    return cf;
}



static FONT *upgrade_to_color(FONT* f)
{
    FONT_MONO_DATA* mf = f->data;
    FONT_COLOR_DATA *cf, *cf_write = 0;
    FONT *outf;

    if (is_color_font(f)) return NULL;
    outf = al_calloc(1, sizeof *outf);
    outf->height = f->height;
    outf->is_color = true;

    while(mf) {
        FONT_MONO_DATA* mf_next = mf->next;

        cf = upgrade_to_color_data(mf);
        if(!cf_write) outf->data = cf;
        else cf_write->next = cf;

        cf_write = cf;
        mf = mf_next;
    }
    
    return outf;
}

/* color_copy_glyph_range:
 *  Colour font helper function. Copies (part of) a glyph range
 */
static FONT_COLOR_DATA *color_copy_glyph_range(FONT_COLOR_DATA *cf, int begin, int end)
{
   FONT_COLOR_DATA *newcf;
   BITMAP **gl;
   BITMAP *g;
   int num, c;
   
   if (begin<cf->begin || end>cf->end)
      return NULL;
   
   newcf = _AL_MALLOC(sizeof *newcf);

   if (!newcf)
      return NULL;

   newcf->begin = begin;
   newcf->end = end;
   newcf->next = NULL;
   num = end - begin;

   gl = newcf->bitmaps = _AL_MALLOC(num * sizeof *gl);
   for (c=0; c<num; c++) {
      g = cf->bitmaps[begin - cf->begin + c];
      gl[c] = create_bitmap_ex(bitmap_color_depth(g), g->w, g->h);
      int y;
      for (y = 0; y < g->h; y++){
          memcpy(gl[c]->line[y], g->line[y], g->w);
      }
      //blit(g, gl[c], 0, 0, 0, 0, g->w, g->h);
   }

   return newcf;
}



/* color_extract_font_range:
 *  (color vtable entry)
 *  Extract a range of characters from a color font 
 */
FONT *extract_font_range(FONT *f, int begin, int end)
{
   FONT *fontout = NULL;
   FONT_COLOR_DATA *cf, *cfin;
   int first, last;

   if (!f)
      return NULL;

   /* Special case: copy entire font */
   if (begin==-1 && end==-1) {
   }
   /* Copy from the beginning */
   else if (begin == -1 && end > color_get_font_range_begin(f, -1)) {
   }
   /* Copy to the end */
   else if (end == -1 && begin <= color_get_font_range_end(f, -1)) {
   }
   /* begin cannot be bigger than end */
   else if (begin <= end && begin != -1 && end != -1) {
   }
   else {
      return NULL;
   }

   /* Get output font */
   fontout = al_calloc(1, sizeof *fontout);

   fontout->height = f->height;
   fontout->data = NULL;
   fontout->is_color = true;

   /* Get real character ranges */
   first = MAX(begin, color_get_font_range_begin(f, -1));
   last = (end>-1) ? MIN(end, color_get_font_range_end(f, -1)) : color_get_font_range_end(f, -1);
   last++;

   cf = NULL;
   cfin = f->data;
   while (cfin) {
      /* Find the range that is covered by the requested range. */
      /* Check if the requested and processed ranges at least overlap */
      if (((first >= cfin->begin && first < cfin->end) || (last <= cfin->end && last > cfin->begin))
      /* Check if the requested range wraps processed ranges */
      || (first < cfin->begin && last > cfin->end)) {
         int local_begin, local_end;
         
         local_begin = MAX(cfin->begin, first);
         local_end = MIN(cfin->end, last);

         if (cf) {
            cf->next = color_copy_glyph_range(cfin, local_begin, local_end);
            cf = cf->next;
         }
         else {
            cf = color_copy_glyph_range(cfin, local_begin, local_end);
            fontout->data = cf;
         }
      }
      cfin = cfin->next;
   }

   return fontout;
}



/* color_merge_fonts:
 *  (color vtable entry)
 *  Merges font2 with font1 and returns a new font
 */
FONT *merge_fonts(FONT *font1, FONT *font2)
{
   FONT *fontout = NULL, *font2_upgr = NULL;
   FONT_COLOR_DATA *cf, *cf1, *cf2;
   
   if (!font1 || !font2)
      return NULL;

   if (!is_color_font(font1))
      return NULL;
   
   if (is_mono_font(font2)) {
      font2_upgr = upgrade_to_color(font2);
      /* Couldn't update font */
      if (!font2_upgr)
         return NULL;
   }
   else
      font2_upgr = font2;

   if (!is_color_font(font2_upgr))
      return NULL;

   /* Get output font */
   fontout = al_calloc(1, sizeof *fontout);
   fontout->height = MAX(font1->height, font2->height);
   fontout->is_color = true;
   cf = fontout->data = NULL;
   
   cf1 = font1->data;
   cf2 = font2_upgr->data;
   while (cf1 || cf2) {
      if (cf1 && (!cf2 ||  (cf1->begin < cf2->begin))) {
         if (cf) {
            cf->next = color_copy_glyph_range(cf1, cf1->begin, cf1->end);
            cf = cf->next;
         }
         else {
            cf = color_copy_glyph_range(cf1, cf1->begin, cf1->end);
            fontout->data = cf;
         }
         cf1 = cf1->next;
      }
      else {
         if (cf) {
            cf->next = color_copy_glyph_range(cf2, cf2->begin, cf2->end);
            cf = cf->next;
         }
         else {
            cf = color_copy_glyph_range(cf2, cf2->begin, cf2->end);
            fontout->data = cf;
         }
         cf2 = cf2->next;
      }
   }

   if (font2_upgr != font2)
      destroy_font(font2_upgr);

   return fontout;
}



/* color_transpose_font:
 *  (color vtable entry)
 *  Transpose all glyphs in a font
 */
int transpose_font(FONT* f, int drange)
{
   FONT_COLOR_DATA* cf = 0;

   if (!f) 
      return -1;

   cf = (FONT_COLOR_DATA*)(f->data);

   while(cf) {
      FONT_COLOR_DATA* next = cf->next;
      
      cf->begin += drange;
      cf->end += drange;
      cf = next;
   }

   return 0;
}


void destroy_font(FONT *f){
    if (!f)
        return;
    if (f->real)
        al_destroy_font(f->real);
    al_free(f);
}

/* font_find_character & bitmap_font_count are taken from A5/addons/font/fontbmp.c verbatim  */
static void font_find_character(uint32_t *data, int pitch,
    int bmp_w, int bmp_h,
    int *x, int *y, int *w, int *h)
{
    /* The pixel at position 0/0 is used as background color. */
    uint32_t c = data[0];
    pitch >>= 2;

    /* look for top left corner of character */
    while (1) {
        /* Reached border? */
        if (*x >= bmp_w - 1) {
            *x = 0;
            (*y)++;
            if (*y >= bmp_h - 1) {
                *w = 0;
                *h = 0;
                return;
            }
        }
        if (
            data[*x + *y * pitch] == c &&
            data[(*x + 1) + *y * pitch] == c &&
            data[*x + (*y + 1) * pitch] == c &&
            data[(*x + 1) + (*y + 1) * pitch] != c) {
            break;
        }
        (*x)++;
    }

    /* look for right edge of character */
    *w = 1;
    while ((*x + *w + 1 < bmp_w) &&
        data[(*x + *w + 1) + (*y + 1) * pitch] != c) {
        (*w)++;
    }

    /* look for bottom edge of character */
    *h = 1;
    while ((*y + *h + 1 < bmp_h) &&
        data[*x + 1 + (*y + *h + 1) * pitch] != c) {
        (*h)++;
    }
}

/* bitmap_font_count:
 *  Helper for `import_bitmap_font', below.
 */
static int bitmap_font_count(ALLEGRO_BITMAP *bmp)
{
    int x = 0, y = 0, w = 0, h = 0;
    int num = 0;
    ALLEGRO_LOCKED_REGION *lock;

    lock = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_RGBA_8888,
        ALLEGRO_LOCK_READONLY);

    while (1) {
        font_find_character(lock->data, lock->pitch,
            al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
            &x, &y, &w, &h);
        if (w <= 0 || h <= 0)
            break;
        num++;
        x += w;
    }

    al_unlock_bitmap(bmp);

    return num;
}

/* import routine for the Allegro .pcx font format heavily based on al_grab_font_from_bitmap */
FONT *load_bitmap_font(AL_CONST char *fname, RGB *pal, void *param){
    ALLEGRO_BITMAP *import_bmp;
    ALLEGRO_STATE backup;
    int range[2];
    PACKFILE *p = pack_fopen(fname, F_READ);
    if (!p)
        return NULL;
    const char *ident = strrchr(fname, '.');
    al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
    import_bmp = al_load_bitmap_flags_f((ALLEGRO_FILE *)p, ident, 0);
    al_restore_state(&backup);
    pack_fclose(p);
    if (!import_bmp) {
        ALLEGRO_ERROR("Couldn't load bitmap from '%s'\n", fname);
        return NULL;
    }

    FONT *f = al_calloc(1, sizeof * f);
    f->data = NULL;
    f->is_color = false;

    /* We assume a single unicode range, starting at the space
     * character.
     */
    range[0] = 32;
    range[1] = 32 + bitmap_font_count(import_bmp) - 1;

    al_convert_mask_to_alpha(import_bmp, al_map_rgb(255, 0, 255));
    /* TODO: There are some fonts with non-transparent bg (blue 0x0000aa on black) like in agup
     * maybe fix things up manually as we count characters instead? */
    al_convert_mask_to_alpha(import_bmp, al_map_rgb(0, 0, 0));

    f->real = al_grab_font_from_bitmap(import_bmp, 1, range);

    al_destroy_bitmap(import_bmp);

    return f;
}

FONT *load_font(char const *name, RGB *pal, void *param){
    FONT *f;
    int len = strlen(name);
    if (len >= 4 && !strcmp(name + len - 4, ".dat")){
        f = al_calloc(1, sizeof *f);
        DATAFILE *dat = load_datafile(name);
        FONT *df = dat->dat;
        f->data = df->data;
        if (!f->is_color) {
           FONT *u = upgrade_to_color(f);
           *f = *u;
        }
    } else
        f = load_bitmap_font(name, pal, param);
    
    return f;
}

/* is_color_font:
 *  returns non-zero if the font passed is a bitmapped colour font
 */
int is_color_font(FONT *f)
{
   ASSERT(f);
   
   return f->is_color;
}



/* is_mono_font:
 *  returns non-zero if the font passed is a monochrome font
 */
int is_mono_font(FONT *f)
{
   ASSERT(f);
   
   return !f->is_color;
}
