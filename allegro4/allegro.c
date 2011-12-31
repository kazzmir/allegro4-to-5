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

#include "include/internal/aintern.h"

#define KEYBUFFER_LENGTH 256

typedef struct {int keycode, unicode, modifiers;} KEYBUFFER_ENTRY;

volatile long midi_pos;
int gfx_capabilities;
volatile int retrace_count;

int _allegro_errno;
int * allegro_errno = &_allegro_errno;
char allegro_id[] = "Allegro 4 to 5 Layer Version 0.1";
char allegro_error[ALLEGRO_ERROR_SIZE];

volatile char key[KEY_MAX];
static volatile KEYBUFFER_ENTRY keybuffer[KEYBUFFER_LENGTH];
static volatile int keybuffer_pos;
volatile int key_shifts;
SYSTEM_DRIVER _system_driver = {0, "A5", "A5", "A5"};
SYSTEM_DRIVER *system_driver = &_system_driver;
KEYBOARD_DRIVER _keyboard_driver = {0, "A5", "A5", "A5", 0};
KEYBOARD_DRIVER *keyboard_driver = &_keyboard_driver;
MOUSE_DRIVER _mouse_driver = {0, "A5", "A5", "A5"};
MOUSE_DRIVER *mouse_driver = &_mouse_driver;
void (*keyboard_lowlevel_callback)(int scancode);
BITMAP * screen;
static FONT _font;
struct FONT * font = &_font;
int * palette_color;
ALLEGRO_DISPLAY * display;
volatile int mouse_w;
volatile int mouse_x;
volatile int mouse_y;
volatile int mouse_z;
volatile int mouse_b;
static int mickey_x, mickey_y;
GFX_DRIVER * gfx_driver;
int current_depth = 8;
static ALLEGRO_MOUSE_CURSOR *cursor;
static ALLEGRO_BITMAP *cursor_bitmap;
static int cursor_x, cursor_y;
static ALLEGRO_CONFIG *current_config;
static ALLEGRO_CONFIG **config_stack;
static int config_stack_size;
int _gfx_mode_set_count;
int _allegro_count;
static bool write_alpha;

JOYSTICK_INFO joy[MAX_JOYSTICKS];
int num_joysticks;

PALETTE current_palette;

RGB_MAP * rgb_map;
COLOR_MAP * color_map;

int palette_color8[256];

/* lookup table for scaling 5 bit colors up to 8 bits */
int _rgb_scale_5[32] =
{
   0,   8,   16,  24,  33,  41,  49,  57,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   198, 206, 214, 222, 231, 239, 247, 255
};


/* lookup table for scaling 6 bit colors up to 8 bits */
int _rgb_scale_6[64] =
{
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   195, 199, 203, 207, 211, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};


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

/* returns an A5 color (ALLEGRO_COLOR) given an A4 packed int */
static ALLEGRO_COLOR a5color(int a4color, int bit_depth){
    if (bit_depth == 8){
        RGB * rgb = &current_palette[a4color];
        return al_map_rgb(rgb->r * 4, rgb->g * 4, rgb->b * 4);
    }
    /* FIXME: handle other depths */
    return al_map_rgb(1, 1, 1);
}

/* returns an a4 color (packed int) given an A5 ALLEGRO_COLOR */
static int a4color(ALLEGRO_COLOR color, int bit_depth){
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return makecol_depth(bit_depth, red, green, blue);
}

int getr_depth(int color_depth, int c){
    ALLEGRO_COLOR color = a5color(c, color_depth);
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return red;
}

int getr(int color){
    return getr_depth(current_depth, color);
}

int getg_depth(int color_depth, int c){
    ALLEGRO_COLOR color = a5color(c, color_depth);
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return green;
}

int getg(int color){
    return getg_depth(current_depth, color);
}

int getb_depth(int color_depth, int c){
    ALLEGRO_COLOR color = a5color(c, color_depth);
    unsigned char red, green, blue;
    al_unmap_rgb(color, &red, &green, &blue);
    return blue;
}

int getb(int color){
    return getb_depth(current_depth, color);
}

int bitmap_mask_color(BITMAP *b){
    return 0;
}

int poll_mouse(){
    return 0;
}

void get_mouse_mickeys(int *x, int *y){
    *x = mouse_x - mickey_x;
    *y = mouse_y - mickey_y;
    mickey_x = mouse_x;
    mickey_y = mouse_y;
}

void set_mouse_sprite(BITMAP *sprite){
    if (cursor) al_destroy_mouse_cursor(cursor);
    cursor_bitmap = sprite->real;
    cursor = al_create_mouse_cursor(cursor_bitmap, cursor_x, cursor_y);
    al_set_mouse_cursor(display, cursor);
}

void set_mouse_sprite_focus(int x, int y){
    if (cursor) al_destroy_mouse_cursor(cursor);
    cursor_x = x;
    cursor_y = y;
    cursor = al_create_mouse_cursor(cursor_bitmap, cursor_x, cursor_y);
    al_set_mouse_cursor(display, cursor);
}

int keypressed(){
    return keybuffer_pos > 0;
}

int ureadkey(int *scancode){
    while (keybuffer_pos == 0)
        al_rest(0.1);
    keybuffer_pos--;
    if (scancode) *scancode = keybuffer[keybuffer_pos].keycode;
    /* FIXME: not sure if the key_shifts should be updated here */
    key_shifts = keybuffer[keybuffer_pos].modifiers;
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

void simulate_keypress(int k){
    if (keybuffer_pos < KEYBUFFER_LENGTH) {
        keybuffer[keybuffer_pos].unicode = k & 255;
        keybuffer[keybuffer_pos].keycode = k >> 8;
        keybuffer[keybuffer_pos].modifiers = 0;
        keybuffer_pos++;
    }
}

void rest(unsigned int milliseconds){
    al_rest(milliseconds / 1000.0);
}

void rest_callback(unsigned int milliseconds, void (*callback)()){
    al_rest(milliseconds / 1000.0);
}

void vsync(){
}

void show_mouse(struct BITMAP *bmp){
}

void scare_mouse(void){
}

void unscare_mouse(void){
}

void scare_mouse_area(int x, int y, int w, int h){
}

void set_trans_blender(int r, int g, int b, int a){
}

void create_rgb_table(RGB_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))){
}

void create_light_table(COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))){
}

static void lazily_create_real_bitmap(BITMAP *bitmap, int is_mono_font){
    if (bitmap->real) return;
    bitmap->real = al_create_bitmap(bitmap->w, bitmap->h);
    ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bitmap->real,
        ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
    char *rgba = lock->data;
    if (bitmap->depth == 8){
        int x, y;
        for (y = 0; y < bitmap->h; y++){
            for (x = 0; x < bitmap->w; x++){
                int c = *(bitmap->line[y] + x);
                unsigned char red = current_palette[c].r * 4;
                unsigned char green = current_palette[c].g * 4;
                unsigned char blue = current_palette[c].b * 4;
                char alpha = 255;
                if (is_mono_font && c)
                    red = green = blue = 255;
                rgba[y * lock->pitch + x * 4 + 0] = red;
                rgba[y * lock->pitch + x * 4 + 1] = green;
                rgba[y * lock->pitch + x * 4 + 2] = blue;
                rgba[y * lock->pitch + x * 4 + 3] = alpha;
            }
        }
    }
    al_unlock_bitmap(bitmap->real);
}

static void lazily_create_real_font(FONT *f){
    if (f->real) return;
    if (!f->data) return;
    // FIXME: additional glyph ranges
    FONT_COLOR_DATA *cf = f->data;
    int maxchars = cf->end - cf->begin;
    int i, w = 0, h = 0;
    for (i = 0; i < maxchars; i++) {
        w += cf->bitmaps[i]->w;
        h = MAX(h, cf->bitmaps[i]->h);
    }

    ALLEGRO_BITMAP *sheet = al_create_bitmap(w + maxchars + 1, h + 2);
   
    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_BLENDER);
    al_set_target_bitmap(sheet);
    al_clear_to_color(al_map_rgba(255, 255, 0, 0));
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
    int x = 1;
    for (i = 0; i < maxchars; i++) {
        lazily_create_real_bitmap(cf->bitmaps[i], 1);
        al_draw_bitmap(cf->bitmaps[i]->real, x, 1, 0);
        x += cf->bitmaps[i]->w + 1;
    }
    al_restore_state(&state);
    int ranges[] = {cf->begin, cf->end - 1};
    f->real = al_grab_font_from_bitmap(sheet, 1, ranges);
    al_destroy_bitmap(sheet);
}

BITMAP * create_bitmap_ex(int depth, int width, int height){
    BITMAP * bitmap = al_calloc(1, sizeof(BITMAP));
    int i;
    bitmap->w = width;
    bitmap->h = height;
    bitmap->depth = depth;
    int size = (depth + 7) / 8;
    /* always use 32-bit RGBA for dat? */
    bitmap->dat = al_malloc(bitmap->w * bitmap->h * size);
    bitmap->line = al_malloc(bitmap->h * sizeof *bitmap->line);
    for (i = 0; i < bitmap->h; i++) {
        bitmap->line[i] = bitmap->dat;
        bitmap->line[i] += i * size * bitmap->w;
    }
    return bitmap;
}

BITMAP * create_bitmap(int width, int height){
    return create_bitmap_ex(current_depth, width, height);
}

static BITMAP * create_bitmap_from(ALLEGRO_BITMAP * real){
    BITMAP *bitmap = create_bitmap_ex(32, al_get_bitmap_width(real),
        al_get_bitmap_height(real));
    bitmap->real = real;
    return bitmap;
}

BITMAP * load_bitmap(const char * path,struct RGB *pal){
    return create_bitmap_from(al_load_bitmap(path));
}

struct BITMAP * load_bmp(AL_CONST char *filename, struct RGB *pal){
    return load_bitmap(filename, pal);
}

void destroy_bitmap(BITMAP* bitmap){
    if (bitmap->real) al_destroy_bitmap(bitmap->real);
    al_free(bitmap);
}

int is_same_bitmap(BITMAP *b1, BITMAP *b2){
    if (!b1 || !b2) return 0;
    return b1->real == b2->real;
}

int poll_keyboard(){
    return 0;
}

static void setup_default_driver(BITMAP * screen){
    gfx_driver = al_malloc(sizeof(GFX_DRIVER));
    memset(gfx_driver, 0, sizeof(GFX_DRIVER));
    gfx_driver->w = screen->w;
    gfx_driver->h = screen->h;
}

/* I think this method can be called after some colors are already generated.
 */
void set_color_depth(int depth){
    current_depth = depth;
}

void set_color_conversion(int mode){
}

int set_gfx_mode(int card, int width, int height, int virtualwidth, int virtualheight){
    int i;
    display = al_create_display(width, height);
    screen = create_bitmap_from(al_get_backbuffer(display));
    palette_color = palette_color8;
    set_palette(default_palette);
    setup_default_driver(screen);
    for (i = 0; i < 256; i++){
        palette_color8[i] = i;
    }
    _gfx_mode_set_count++;
    return is_ok(display != NULL);
}

/*
void set_projection_viewport(){
}
*/

void clear_bitmap(BITMAP * bitmap){
    lazily_create_real_bitmap(bitmap, 0);
    al_set_target_bitmap(bitmap->real);
    al_clear_to_color(al_map_rgb(0, 0, 0));
}

void allegro_message(char const *format, ...){
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int install_timer(){
    return 0;
}

int install_mouse(){
    return 2;
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

int scancode_to_ascii(int scancode){
    return '?';
}

static int is_shift(int key){
    return key == ALLEGRO_KEY_LSHIFT ||
           key == ALLEGRO_KEY_RSHIFT;
}

static void * read_keys(ALLEGRO_THREAD * self, void * arg){

    ALLEGRO_EVENT_QUEUE * queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

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
                /* FIXME: handle the rest of the modifiers */
                keybuffer[keybuffer_pos].modifiers = (event.keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? KB_SHIFT_FLAG : 0;
                keybuffer_pos++;
            }
        } else if (event.type == ALLEGRO_EVENT_MOUSE_AXES){
            mouse_w = event.mouse.w;
            mouse_x = event.mouse.x;
            mouse_y = event.mouse.y;
            mouse_z = event.mouse.z;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN){
            mouse_b |= 1 << (event.mouse.button - 1);
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP){
            mouse_b &= ~(1 << (event.mouse.button - 1));
        }
    }

    return NULL;
}

static void *system_thread(ALLEGRO_THREAD * self, void * arg){

    ALLEGRO_EVENT_QUEUE * queue = al_create_event_queue();

    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60);
    al_start_timer(timer);
    al_register_event_source(queue, al_get_timer_event_source(timer));
    
    while (true){
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_TIMER){
            retrace_count++;
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

static void start_system_thread(){
    ALLEGRO_THREAD * thread = al_create_thread(system_thread, NULL);
    if (thread != NULL){
        al_start_thread(thread);
    } else {
        printf("Could not start system thread!\n");
    }
}

int install_allegro(int system_id, int *errno_ptr, int (*atexit_ptr)(void (*func)(void))){
    return _install_allegro_version_check(system_id, errno_ptr, atexit_ptr, 0);
}

int _install_allegro_version_check(int system_id, int *errno_ptr, int (*atexit_ptr)(void (*func)(void)), int version){
    int index;
    ALLEGRO_PATH *path;
    int ok = al_init();
    current_config = al_create_config();
    al_init_primitives_addon();
    al_init_image_addon();
    al_init_font_addon();
    al_install_keyboard();
    al_install_mouse();
    // al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    start_key_thread();
    start_system_thread();

    for (index = 16; index < 256; index++){
        desktop_palette[index] = desktop_palette[index & 15];
    }
    
    path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
    al_append_path_component(path, "examples/");
    al_set_path_filename(path, "a4_font.tga");
    font->real = al_load_font(al_path_cstr(path, '/'), 0, 0);
    al_destroy_path(path);
    
    _allegro_count++;

    return is_ok(ok);
}

static void draw_into(BITMAP *bitmap){
    lazily_create_real_bitmap(bitmap, 0);
    al_set_target_bitmap(bitmap->real);
}

void circle(BITMAP * buffer, int x, int y, int radius, int color){
    draw_into(buffer);
    al_draw_circle(x, y, radius, a5color(color, current_depth), 1);
}

void circlefill(BITMAP * buffer, int x, int y, int radius, int color){
    draw_into(buffer);
    al_draw_filled_circle(x, y, radius, a5color(color, current_depth));
}

void rect(BITMAP * buffer, int x1, int y1, int x2, int y2, int color){
    draw_into(buffer);
    al_draw_rectangle(x1, y1, x2+1, y2+1, a5color(color, current_depth), 1);
}

void rectfill(BITMAP * buffer, int x1, int y1, int x2, int y2, int color){
    draw_into(buffer);
    al_draw_filled_rectangle(x1, y1, x2+1, y2+1, a5color(color, current_depth));
}

void triangle(BITMAP * buffer, int x1, int y1, int x2, int y2, int x3, int y3, int color){
    draw_into(buffer);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, a5color(color, current_depth));
}

int getpixel(BITMAP * buffer, int x, int y){
    lazily_create_real_bitmap(buffer, 0);
    ALLEGRO_BITMAP * al_buffer = buffer->real;
    return a4color(al_get_pixel(al_buffer, x, y), current_depth);
}

void putpixel(BITMAP * buffer, int x, int y, int color){
    ALLEGRO_STATE state;
    draw_into(buffer);
    if (write_alpha) {
        al_store_state(&state, ALLEGRO_STATE_BLENDER);
        al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE,
            ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
    }
    al_draw_pixel(x + 0.5, y + 0.5, a5color(color, current_depth));
    if (write_alpha) {
        al_restore_state(&state);
    }
}

void line(BITMAP * buffer, int x, int y, int x2, int y2, int color){
    draw_into(buffer);
    al_draw_line(x, y, x2, y2, a5color(color, current_depth), 1);
}

void hline(BITMAP * buffer, int x, int y, int x2, int color){
    line(buffer, x, y, x2, y, color);
}

void vline(BITMAP * buffer, int x, int y, int y2, int color){
    line(buffer, x, y, x, y2, color);
}

void set_palette(const PALETTE palette){
    memcpy(current_palette, palette, sizeof(PALETTE));
}

void get_palette(PALETTE palette){
    memcpy(palette, current_palette, sizeof(PALETTE));
}

static void maybe_flip_screen(BITMAP * where){
    if (where == screen){
        al_flip_display();
    }
}

void stretch_blit(BITMAP *source, BITMAP *dest, int source_x,
    int source_y, int source_width, int source_height, int dest_x,
    int dest_y, int dest_width, int dest_height){
        
    lazily_create_real_bitmap(source, 0);
    lazily_create_real_bitmap(dest, 0);
        
    ALLEGRO_BITMAP * al_from = source->real;
    ALLEGRO_BITMAP * al_to = dest->real;
     /* A4 allows drawing a bitmap to itself, A5 does not. */
     if (al_from == al_to) {
         ALLEGRO_BITMAP *temp = al_create_bitmap(source_width, source_height);
         al_set_target_bitmap(temp);
         al_draw_bitmap(al_from, -source_x, -source_y, 0);
         al_set_target_bitmap(al_to);
         al_draw_scaled_bitmap(temp, 0, 0, source_width,
             source_height, dest_x, dest_y, dest_width, dest_height, 0);
         al_destroy_bitmap(temp);
     } else {
         al_set_target_bitmap(al_to);
         al_draw_scaled_bitmap(al_from, source_x, source_y, source_width,
             source_height, dest_x, dest_y, dest_width, dest_height, 0);
     }
 
    maybe_flip_screen(dest);
}

void blit(BITMAP * from, BITMAP * to, int from_x, int from_y, int to_x, int to_y, int width, int height){
    stretch_blit(from, to, from_x, from_y, width, height, to_x, to_y, width, height);
}

void draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y){
    blit(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h);
}

void draw_trans_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y){
    blit(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h);
}

void draw_lit_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int a){
    blit(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h);
}

void textprintf_ex(struct BITMAP *bmp, struct FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...){
    char buffer[65536];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    textout_ex(bmp, f, buffer, x, y, color, bg);
}

void textout_centre_ex(struct BITMAP *bmp, struct FONT *f, AL_CONST char *str, int x, int y, int color, int bg){
    lazily_create_real_font(f);
    al_set_target_bitmap(bmp->real);
    if (bg != -1) {
        int w = al_get_text_width(f->real, str);
        al_draw_filled_rectangle(x - w / 2, y, x + w - w / 2,
            y + al_get_font_line_height(f->real), a5color(bg, current_depth));
    }
    al_draw_text(f->real, a5color(color, current_depth), x, y, ALLEGRO_ALIGN_CENTRE, str);
}

void textout_right_ex(struct BITMAP *bmp, struct FONT *f, AL_CONST char *str, int x, int y, int color, int bg){
    lazily_create_real_font(f);
    al_set_target_bitmap(bmp->real);
    if (bg != -1) {
        int w = al_get_text_width(f->real, str);
        al_draw_filled_rectangle(x - w / 2, y, x + w - w / 2,
            y + al_get_font_line_height(f->real), a5color(bg, current_depth));
    }
    al_draw_text(f->real, a5color(color, current_depth), x, y, ALLEGRO_ALIGN_RIGHT, str);
}

void textprintf_right_ex(struct BITMAP *bmp, struct FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...){
    char buffer[65536];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    textout_right_ex(bmp, f, buffer, x, y, color, bg);
}

void textprintf_centre_ex(struct BITMAP *bmp, struct FONT *f, int x, int y, int color, int bg, AL_CONST char *format, ...){
    char buffer[65536];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    textout_centre_ex(bmp, f, buffer, x, y, color, bg);
}

void textout_ex(struct BITMAP *bmp, struct FONT *f, AL_CONST char *str, int x, int y, int color, int bg){
    lazily_create_real_font(f);
    draw_into(bmp);
    if (bg != -1) {
        al_draw_filled_rectangle(x, y, x + al_get_text_width(f->real, str),
            y + al_get_font_line_height(f->real), a5color(bg, current_depth));
    }
    al_draw_text(f->real, a5color(color, current_depth), x, y, 0, str);

    maybe_flip_screen(bmp);
}

void draw_gouraud_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4){
    draw_into(bmp);
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
    lazily_create_real_bitmap(bitmap, 0);
    al_set_target_bitmap(bitmap->real);
    al_set_clipping_rectangle(x1, y1, x2 - x1, y2 - y1);
}

void set_clip_state(BITMAP * bitmap, int onoff){

}

int bitmap_color_depth(BITMAP * bitmap){
    /* FIXME: return the depth of the bitmap */
    return current_depth;
}

int makecol(int r, int g, int b){
    return makecol_depth(current_depth, r, g, b);
}

void clear_to_color(BITMAP *bitmap, int color){
    draw_into(bitmap);
    al_clear_to_color(a5color(color, current_depth));
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
        // a5_vertexes[index].z = vtx[index]->z;
        /* FIXME: z depends on the camera, or something */
        a5_vertexes[index].z = 0;
        a5_vertexes[index].u = vtx[index]->u;
        a5_vertexes[index].v = vtx[index]->v;
        a5_vertexes[index].color = a5color(color, current_depth);
        
        /*
        printf("%d: x %f y %f z %f u %f v %f\n",
               index,
               vtx[index]->x,
               vtx[index]->y,
               vtx[index]->z,
               vtx[index]->u,
               vtx[index]->v);
               */
    }

    al_set_target_bitmap(bitmap->real);
    if (texture != NULL){
        a5_texture = texture->real;
    }

    /* FIXME: handle the 'type' parameter */
    al_draw_prim(a5_vertexes, NULL, a5_texture, 0, vc, ALLEGRO_PRIM_TRIANGLE_FAN);

    free(a5_vertexes);
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

int install_int(void (*proc)(void), long speed){
    return install_int_ex(proc, MSEC_TO_TIMER(speed));
}

void push_config_state(){
    config_stack_size++;
    config_stack = al_realloc(config_stack, config_stack_size * sizeof *config_stack);
    config_stack[config_stack_size - 1] = current_config;
    current_config = al_create_config();
}

void pop_config_state(){
    if (current_config) al_destroy_config(current_config);
    current_config = config_stack[config_stack_size - 1];
    config_stack_size--;
    config_stack = al_realloc(config_stack, config_stack_size * sizeof *config_stack);
}

void set_config_file(char const *filename){
    if (current_config) al_destroy_config(current_config);
    current_config = al_load_config_file(filename);
    if (!current_config) current_config = al_create_config();
}

static char _tokens[64][64];
static char *tokens[64];
char **get_config_argv(char const *section, char const *name, int *argc){
    char const *v;
    int i, pos = 0;
    v = al_get_config_value(current_config, section, name);
    *argc = 0;
    _tokens[0][0] = 0;
    tokens[0] = _tokens[0];
    if (!v) return tokens;
    while (*argc < 64) {
        for (i = 0; i < 63; i++) {
            char c = v[pos];
            if (!c) break;
            pos++;
            if (c == ' ') break;
            _tokens[*argc][i] = c;
        }
        _tokens[*argc][i] = 0;
        tokens[*argc] = _tokens[*argc];
        (*argc)++;
        if (!v[pos]) break;
    }
    
    return tokens;
}

char const *get_config_string(char const *section, char const *name, char const *def){
    char const *v = al_get_config_value(current_config, section, name);
    if (!v) return def;
    return v;
}

int get_config_int(char const *section, char const *name, int def){
    char const *v = al_get_config_value(current_config, section, name);
    if (!v) return def;
    return strtol(v, NULL, 10);
}

unsigned int _default_ds(){
    return 0;
}

void _al_getdcwd(int drive, char *buf, int size){
    char *d = al_get_current_directory();
    strncpy(buf, d, size);
    al_free(d);
}

int _al_file_isok(char const *filename){
    return 1;
}

char *_al_strdup(char const *s){
    return strdup(s);
}

void get_executable_name(char *buf, int size){
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
    strncpy(buf, al_path_cstr(path, '/'), size);
    al_destroy_path(path);
}

int _color_load_depth(int depth, int hasalpha){
    return depth;
}

int request_video_bitmap(struct BITMAP *bitmap){
    /* FIXME */
    return -1;
}

int poll_scroll(){
    return -1;
}

BITMAP * create_video_bitmap(int width, int height){
    return create_bitmap(width, height);
}

int enable_triple_buffer(){
    /* FIXME */
    return -1;
}

int save_bitmap(AL_CONST char *filename, struct BITMAP *bmp, AL_CONST struct RGB *pal){
    /* FIXME */
    return -1;
}

int is_video_bitmap(BITMAP * what){
    /* All bitmaps are video */
    return 1;
}

void rotate_scaled_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale){
    /* FIXME */
}

void rotate_scaled_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale){
    /* FIXME */
}

void solid_mode(){
    write_alpha = false;
}

void drawing_mode(int mode, struct BITMAP *pattern, int x_anchor, int y_anchor){
    write_alpha = false;
}

void quad3d(struct BITMAP *bmp, int type, struct BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4){
    /* FIXME */
}

void set_write_alpha_blender(void){
    write_alpha = true;
}

void set_multiply_blender(int r, int g, int b, int a){
    write_alpha = false;
}

void set_alpha_blender(void){
    write_alpha = false;
}

int show_video_bitmap(BITMAP *bitmap){
    blit(bitmap, screen, 0, 0, 0, 0, bitmap->w, bitmap->h);
    return 0;
}

int play_sample(AL_CONST SAMPLE * sample, int volume, int pan, int frequency, int loop){
    /* FIXME */
    return -1;
}

void stop_midi(){
    /* FIXME */
}

int play_midi(MIDI *midi, int loop){
    /* FIXME */
    return -1;
}

void set_volume(int digi_volume, int midi_volume){
    /* FIXME */
}

int text_length(AL_CONST struct FONT *f, AL_CONST char *str){
    /* FIXME */
    return -1;
}

int text_height(AL_CONST struct FONT *f){
    /* FIXME */
    return -1;
}

void set_keyboard_rate(int delay, int repeat){
    /* FIXME */
}

void set_config_int(AL_CONST char *section, AL_CONST char *name, int val){
    /* FIXME */
}

int show_os_cursor(int cursor){
    /* FIXME */
    return -1;
}

int desktop_color_depth(void){
    /* FIXME */
    return -1;
}

void voice_start(int voice){
    /* FIXME */
}

void voice_set_volume(int voice, int volume){
    /* FIXME */
}

void voice_set_playmode(int voice, int playmode){
    /* FIXME */
}

int allocate_voice(AL_CONST SAMPLE *spl){
    /* FIXME */
    return -1;
}

void deallocate_voice(int voice){
    /* FIXME */
}

void voice_stop(int voice){
    /* FIXME */
}

int poll_joystick(void){
    /* FIXME */
    return 0;
}

void remove_int(void (*proc)(void)){
    /* FIXME */
}

void release_bitmap(BITMAP * bitmap){
}

void acquire_bitmap(BITMAP * bitmap){
    /* We can't do anything here as it does some quite special things in A4.
     * E.g. under DirectX it's required in order to use HW accel, sometimes
     * it just speeds up putpixel, ...
     */
}

int install_joystick(int type){
    /* FIXME */
    return -1;
}

int set_display_switch_mode(int mode){
    /* FIXME */
    return -1;
}

int install_sound(int digi, int midi, AL_CONST char *cfg_path){
    /* FIXME */
    return -1;
}

int set_close_button_callback(void (*proc)(void)){
    /* FIXME */
    return -1;
}

void set_window_title(AL_CONST char *name){
    /* FIXME */
}

int stricmp(AL_CONST char *s1, AL_CONST char *s2){
    /* FIXME */
    return -1;
}

void release_voice(int voice){
    /* FIXME */
}

void destroy_font(FONT *f){
    al_destroy_font(f->real);
    al_free(f);
}

void destroy_rle_sprite(RLE_SPRITE *rle){
    destroy_bitmap(rle);
}

void destroy_compiled_sprite(COMPILED_SPRITE *sprite){
    destroy_bitmap(sprite);
}

void draw_rle_sprite(BITMAP *bmp, RLE_SPRITE *sprite, int x, int y){
    draw_sprite(bmp, sprite, x, y);
}

RLE_SPRITE *get_rle_sprite(BITMAP *bitmap){
    return bitmap;
}

COMPILED_SPRITE *get_compiled_sprite(BITMAP *bitmap, int planar){
    return bitmap;
}

void *_al_sane_realloc(void *ptr, size_t size){
    return al_realloc(ptr, size);
}

int set_display_switch_callback(int dir, void (*cb)()){
    return 0;
}

int get_display_switch_mode(){
    return SWITCH_BACKGROUND;
}

void remove_display_switch_callback(void (*cb)()){
}
