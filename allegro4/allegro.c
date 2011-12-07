#include <stdio.h>
#include <stdlib.h>
#include "allegro.h"

/* FIXME */
int AL_RAND(){
    return rand();
}

#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#define KEYBUFFER_LENGTH 256

typedef struct {int keycode, unicode, modifiers;} KEYBUFFER_ENTRY;

int * allegro_errno;
int allegro_error;
volatile char key[KEY_MAX];
static volatile KEYBUFFER_ENTRY keybuffer[KEYBUFFER_LENGTH];
static volatile int keybuffer_pos;
volatile int key_shifts;
KEYBOARD_DRIVER _keyboard_driver = {0, "A5", "A5", "A5", 0};
KEYBOARD_DRIVER *keyboard_driver = &_keyboard_driver;
void (*keyboard_lowlevel_callback)(int scancode);
BITMAP * screen;
static FONT _font;
struct FONT * font = &_font;
int * palette_color;
ALLEGRO_DISPLAY * display;
int mouse_x;
int mouse_y;
GFX_DRIVER * gfx_driver;
static int current_depth = 8;

PALETTE current_palette;

RGB_MAP * rgb_map;
COLOR_MAP * color_map;

int palette_color8[256];

/*
struct BITMAP{
    ALLEGRO_BITMAP * real;
    int w;
    int h;
};
*/

/* allegro4 uses 0 as ok values */
static int is_ok(int code){
    if (code){
        return 0;
    }
    return -1;
}

static ALLEGRO_COLOR a4color(int color, int bit_depth){
    if (bit_depth == 8){
        RGB * rgb = &current_palette[color];
        return al_map_rgb(rgb->r * 4, rgb->g * 4, rgb->b * 4);
    }
    /* FIXME: handle other depths */
    return al_map_rgb(1, 1, 1);
}

int getr_depth(int color_depth, int c){
    ALLEGRO_COLOR color = a4color(c, color_depth);
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return red;
}

int getg_depth(int color_depth, int c){
    ALLEGRO_COLOR color = a4color(c, color_depth);
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return green;
}

int getb_depth(int color_depth, int c){
    ALLEGRO_COLOR color = a4color(c, color_depth);
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return blue;
}

void poll_mouse(){
}

int keypressed(){
    return keybuffer_pos > 0;
}

int ureadkey(int *scancode){
    while (keybuffer_pos == 0)
        al_rest(0.1);
    keybuffer_pos--;
    if (scancode) *scancode = keybuffer[keybuffer_pos].keycode;
    return keybuffer[keybuffer_pos].unicode;
}

int readkey(){
    int scancode;
    int c = ureadkey(&scancode);
    return (c & 255) + (scancode << 8);
}

void clear_keybuf(){
    keybuffer_pos = 0;
}

void rest(int milliseconds){
    al_rest(milliseconds / 1000.0);
}

void vsync(){
}

void show_mouse(){
}

void set_trans_blender(int r, int g, int b, int a){
}

void create_rgb_table(RGB_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))){
}

void create_light_table(COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))){
}

static BITMAP * create_bitmap_from(ALLEGRO_BITMAP * real){
    BITMAP * bitmap = malloc(sizeof(BITMAP));
    bitmap->real = real;
    bitmap->w = al_get_bitmap_width(real);
    bitmap->h = al_get_bitmap_height(real);
    return bitmap;
}

BITMAP * load_bitmap(const char * path,struct RGB *pal){
    return create_bitmap_from(al_load_bitmap(path));
}

void destroy_bitmap(BITMAP* bitmap){
    al_destroy_bitmap(bitmap->real);
    free(bitmap);
}

int poll_keyboard(){
    return 0;
}

static void setup_default_driver(BITMAP * screen){
    gfx_driver = al_malloc(sizeof(GFX_DRIVER));
    memset(gfx_driver, 0, sizeof(GFX_DRIVER));
    gfx_driver->w = al_get_bitmap_width(screen->real);
    gfx_driver->h = al_get_bitmap_height(screen->real);
}

/* I think this method can be called after some colors are already generated.
 */
void set_color_depth(int depth){
    current_depth = depth;
}

int set_gfx_mode(int card, int width, int height, int virtualwidth, int virtualheight){
    int i;
    display = al_create_display(width, height);
    screen = create_bitmap_from(al_get_backbuffer(display));
    palette_color = palette_color8;
    setup_default_driver(screen);
    for (i = 0; i < 256; i++){
        palette_color8[i] = i;
    }
    return is_ok(display != NULL);
}

/*
void set_projection_viewport(){
}
*/

void clear_bitmap(BITMAP * bitmap){
    al_set_target_bitmap(bitmap->real);
    al_clear_to_color(al_map_rgb(0, 0, 0));
}

void allegro_message(char const *format, ...){
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

BITMAP * create_bitmap(int width, int height){
    // return (BITMAP*) al_create_bitmap(width, height);
    BITMAP * bitmap = malloc(sizeof(BITMAP));
    bitmap->w = width;
    bitmap->h = height;
    bitmap->real = al_create_bitmap(width, height);
    return bitmap;
}

void install_timer(){
}

void install_mouse(){
}

int install_keyboard(){
    return 0;
}

static int a4key(int a5key){
    switch (a5key){
        case ALLEGRO_KEY_LCTRL: return KEY_LCONTROL;
        case ALLEGRO_KEY_RCTRL: return KEY_RCONTROL;
        default: return a5key;
    }
}

static int a5key(int a4key){
    switch (a4key){
        case KEY_LCONTROL: return ALLEGRO_KEY_LCTRL;
        case KEY_RCONTROL: return ALLEGRO_KEY_RCTRL;
        default: return a4key;
    }
}

char const *scancode_to_name(int scancode){
    return al_keycode_to_name(a5key(scancode));
}

static int is_shift(int key){
    return key == ALLEGRO_KEY_LSHIFT ||
           key == ALLEGRO_KEY_RSHIFT;
}

static void * read_keys(ALLEGRO_THREAD * self, void * arg){
    ALLEGRO_EVENT_QUEUE * queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    while (true){
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_KEY_DOWN){
            int k = a4key(event.keyboard.keycode);
            key[k] = 1;
            if (is_shift(event.keyboard.keycode)){
                key_shifts |= KB_SHIFT_FLAG;
            }

            if (keyboard_lowlevel_callback) {
                al_set_target_backbuffer(display);
                keyboard_lowlevel_callback(k);
            }
        } else if (event.type == ALLEGRO_EVENT_KEY_UP){
            int k = a4key(event.keyboard.keycode);
            key[k] = 0;
            if (is_shift(event.keyboard.keycode)){
                key_shifts &= ~KB_SHIFT_FLAG;
            }
            if (keyboard_lowlevel_callback) {
                al_set_target_backbuffer(display);
                keyboard_lowlevel_callback(k + 128);
            }
        } else if (event.type == ALLEGRO_EVENT_KEY_CHAR){
            if (keybuffer_pos < KEYBUFFER_LENGTH) {
                keybuffer[keybuffer_pos].unicode = event.keyboard.unichar;
                keybuffer[keybuffer_pos].keycode = event.keyboard.keycode;
                keybuffer[keybuffer_pos].modifiers = (is_shift(event.keyboard.keycode) ? KB_SHIFT_FLAG : 0);
                keybuffer_pos++;
            }
        }
    }

    return NULL;
}

static void start_key_thread(){
    ALLEGRO_THREAD * thread = al_create_thread(read_keys, NULL);
    if (thread != NULL){
        al_start_thread(thread);
    } else {
        printf("Could not start key thread!\n");
    }
}

int allegro_init(){
    ALLEGRO_PATH *path;
    allegro_errno = &allegro_error;
    int ok = al_init();
    al_init_primitives_addon();
    al_init_image_addon();
    al_init_font_addon();
    al_install_keyboard();
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    start_key_thread();
    
    path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
    al_append_path_component(path, "examples/");
    al_set_path_filename(path, "a4_font.tga");
    font->real = al_load_font(al_path_cstr(path, '/'), 0, 0);
    al_destroy_path(path);

    return is_ok(ok);
}

void rect(BITMAP * buffer, int x1, int y1, int x2, int y2, int color){
    al_set_target_bitmap(buffer->real);
    al_draw_rectangle(x1, y1, x2+1, y2+1, a4color(color, current_depth), 1);
}

void rectfill(BITMAP * buffer, int x1, int y1, int x2, int y2, int color){
    al_set_target_bitmap(buffer->real);
    al_draw_filled_rectangle(x1, y1, x2+1, y2+1, a4color(color, current_depth));
}

void triangle(BITMAP * buffer, int x1, int y1, int x2, int y2, int x3, int y3, int color){
    al_set_target_bitmap(buffer->real);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, a4color(color, current_depth));
}

void putpixel(BITMAP * buffer, int x, int y, int color){
    ALLEGRO_BITMAP * al_buffer = buffer->real;
    al_set_target_bitmap(al_buffer);
    al_put_pixel(x, y, a4color(color, current_depth));
}

void set_palette(const PALETTE palette){
    memcpy(current_palette, palette, sizeof(PALETTE));
}

void blit(BITMAP * from, BITMAP * to, int from_x, int from_y, int to_x, int to_y, int width, int height){
    ALLEGRO_BITMAP * al_from = from->real;
    ALLEGRO_BITMAP * al_to = to->real;
    /* A4 allows drawing a bitmap to itself, A5 does not. */
    if (al_from == al_to) {
        ALLEGRO_BITMAP *temp = al_create_bitmap(width, height);
        al_set_target_bitmap(temp);
        al_draw_bitmap(al_from, -from_x, -from_y, 0);
        al_set_target_bitmap(al_to);
        al_draw_bitmap(temp, to_x, to_y, 0);
        al_destroy_bitmap(temp);
    }
    else {
        al_set_target_bitmap(al_to);
        al_draw_bitmap(al_from, to_x, to_y, 0);
    }
    if (to == screen){
        al_flip_display();
    }
}

void textprintf_ex(struct BITMAP *bmp, AL_CONST struct FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...){
    char buffer[65536];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    textout_ex(bmp, f, buffer, x, y, color, bg);
}

void textout_centre_ex(struct BITMAP *bmp, AL_CONST struct FONT *f, AL_CONST char *str, int x, int y, int color, int bg){
    al_set_target_bitmap(bmp->real);
    if (bg != -1) {
        int w = al_get_text_width(f->real, str);
        al_draw_filled_rectangle(x - w / 2, y, x + w - w / 2,
            y + al_get_font_line_height(f->real), a4color(bg, current_depth));
    }
    al_draw_text(f->real, a4color(color, current_depth), x, y, ALLEGRO_ALIGN_CENTRE, str);
}

void textprintf_centre_ex(struct BITMAP *bmp, AL_CONST struct FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...){
    char buffer[65536];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    textout_centre_ex(bmp, f, buffer, x, y, color, bg);
}

void textout_ex(struct BITMAP *bmp, AL_CONST struct FONT *f, AL_CONST char *str, int x, int y, int color, int bg){
    al_set_target_bitmap(bmp->real);
    if (bg != -1) {
        al_draw_filled_rectangle(x, y, x + al_get_text_width(f->real, str),
            y + al_get_font_line_height(f->real), a4color(bg, current_depth));
    }
    al_draw_text(f->real, a4color(color, current_depth), x, y, 0, str);
}

void draw_gouraud_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4){
    al_set_target_bitmap(bmp->real);
    al_draw_bitmap(sprite->real, x, y, 0);
}

int makecol_depth(int depth, int r, int g, int b){
    switch (depth){
        case 8: return bestfit_color(current_palette, r>>2, g>>2, b>>2);
        /* FIXME: handle 15, 16, 24, 32 */
        default: return 0;
    }
}

void set_clip_rect(BITMAP * bitmap, int x1, int y1, int x2, int y2){
    al_set_target_bitmap(bitmap->real);
    al_set_clipping_rectangle(x1, y1, x2 - x1, y2 - y1);
}

int bitmap_color_depth(BITMAP * bitmap){
    /* FIXME: return the depth of the bitmap */
    return current_depth;
}

int makecol(int r, int g, int b){
    return makecol_depth(current_depth, r, g, b);
}

void clear_to_color(BITMAP *bitmap, int color){
    al_set_target_bitmap(bitmap->real);
    al_clear_to_color(a4color(color, current_depth));
}

void acquire_screen(){
}

void release_screen(){
}

fixed fixmul(fixed x, fixed y){
    return ftofix(fixtof(x) * fixtof(y));
}

fixed itofix(int x){
   return x << 16;
}

int fixtoi(fixed x){
   return fixfloor(x) + ((x & 0x8000) >> 15);
}

fixed ftofix(double x){
   if (x > 32767.0) {
      *allegro_errno = ERANGE;
      return 0x7FFFFFFF;
   }

   if (x < -32767.0) {
      *allegro_errno = ERANGE;
      return -0x7FFFFFFF;
   }

   return (fixed)(x * 65536.0 + (x < 0 ? -0.5 : 0.5));
}

double fixtof(fixed x){
   return (double)x / 65536.0;
}

int fixfloor(fixed x){
   /* (x >> 16) is not portable */
   if (x >= 0)
      return (x >> 16);
   else
      return ~((~x) >> 16);
}

fixed fixcos(fixed x){
   return _cos_tbl[((x + 0x4000) >> 15) & 0x1FF];
}

fixed fixsin(fixed x){
   return _cos_tbl[((x - 0x400000 + 0x4000) >> 15) & 0x1FF];
}

fixed fixdiv(fixed x, fixed y){
   if (y == 0) {
      *allegro_errno = ERANGE;
      return (x < 0) ? -0x7FFFFFFF : 0x7FFFFFFF;
   }
   else
      return ftofix(fixtof(x) / fixtof(y));
}

#define CALC_ROW(n)     (fixmul(x, m->v[n][0]) +      \
                         fixmul(y, m->v[n][1]) +      \
                         fixmul(z, m->v[n][2]) +      \
                         m->t[n])

void apply_matrix(MATRIX *m, fixed x, fixed y, fixed z, fixed *xout, fixed *yout, fixed *zout){
   *xout = CALC_ROW(0);
   *yout = CALC_ROW(1);
   *zout = CALC_ROW(2);
}

#undef CALC_ROW

fixed dot_product(fixed x1, fixed y_1, fixed z1, fixed x2, fixed y2, fixed z2){
   return fixmul(x1, x2) + fixmul(y_1, y2) + fixmul(z1, z2);
}

float dot_product_f(float x1, float y_1, float z1, float x2, float y2, float z2){
   return (x1 * x2) + (y_1 * y2) + (z1 * z2);
}

void persp_project(fixed x, fixed y, fixed z, fixed *xout, fixed *yout){
   *xout = fixmul(fixdiv(x, z), _persp_xscale) + _persp_xoffset;
   *yout = fixmul(fixdiv(y, z), _persp_yscale) + _persp_yoffset;
}

void persp_project_f(float x, float y, float z, float *xout, float *yout){
   float z1 = 1.0f / z;
   *xout = ((x * z1) * _persp_xscale_f) + _persp_xoffset_f;
   *yout = ((y * z1) * _persp_yscale_f) + _persp_yoffset_f;
}

void polygon3d_f(BITMAP * bitmap, int type, BITMAP * texture, int vc, V3D_f * vtx[]){
    ALLEGRO_VERTEX * a5_vertexes = (ALLEGRO_VERTEX*) malloc(sizeof(ALLEGRO_VERTEX) * vc);
    ALLEGRO_BITMAP * a5_texture = NULL;
    int index;
    int color = vtx[0]->c;
    for (index = 0; index < vc; index++){
        a5_vertexes[index].x = vtx[index]->x;
        a5_vertexes[index].y = vtx[index]->y;
        a5_vertexes[index].z = vtx[index]->z;
        a5_vertexes[index].u = vtx[index]->u;
        a5_vertexes[index].v = vtx[index]->v;
        a5_vertexes[index].color = a4color(color, current_depth);
    }

    al_set_target_bitmap(bitmap->real);
    if (texture != NULL){
        a5_texture = texture->real;
    }
    /* FIXME: handle the 'type' parameter */
    al_draw_prim(a5_vertexes, NULL, a5_texture, 0, vc, ALLEGRO_PRIM_TRIANGLE_FAN);

    free(a5_vertexes);
}

/* Convert A5 times (given as a double) to a long by multiplying by 1000 */
#define TIMERS_PER_SECOND     1193181L
long BPS_TO_TIMER(long time){
    return ALLEGRO_BPS_TO_SECS(time) * 1000;
}

struct timer_stuff{
    long speed;
    void (*callback)();
};

static void * start_timer(ALLEGRO_THREAD * self, void * arg){
    struct timer_stuff * stuff = (struct timer_stuff*) arg;
    ALLEGRO_TIMER * timer;
    double speed = stuff->speed / 1000.0;
    ALLEGRO_EVENT_QUEUE * queue = al_create_event_queue();

    timer = al_create_timer(speed);
    al_start_timer(timer);
    al_register_event_source(queue, al_get_timer_event_source(timer));

    while (true){
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_TIMER){
            stuff->callback();
        }
    }

    free(stuff);
    return NULL;
}

int install_int_ex(void (*proc)(void), long speed){
    struct timer_stuff * timer = malloc(sizeof(struct timer_stuff));
    timer->speed = speed;
    timer->callback = proc;
    ALLEGRO_THREAD * thread = al_create_thread(start_timer, timer);
    if (thread != NULL){
        al_start_thread(thread);
        return 0;
    }
    return -1;
}

