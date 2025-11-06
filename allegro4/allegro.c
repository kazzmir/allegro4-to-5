#include <stdio.h>
#include <stdlib.h>
#include "allegro.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>

#include "include/internal/aintern.h"
#ifdef __AVX__
#include <immintrin.h>
#endif

#define VERSION_5_1_0 0x05010000

ALLEGRO_DEBUG_CHANNEL("a425")

typedef struct TIMER {
    ALLEGRO_TIMER *timer;
    void (*callback)();
} TIMER;

#define KEYBUFFER_LENGTH 256

typedef struct {int keycode, unicode, modifiers;} KEYBUFFER_ENTRY;

volatile long midi_pos;
int gfx_capabilities;
volatile int retrace_count;

int _allegro_errno;
int * allegro_errno = &_allegro_errno;
char allegro_id[] = "Allegro 4 to 5 Layer Version 0.1";
char allegro_error[ALLEGRO_ERROR_SIZE];

static ALLEGRO_THREAD *system_thread;
static ALLEGRO_EVENT_QUEUE *system_event_queue;
static TIMER timers[MAX_TIMERS];
struct ALLEGRO_MUTEX *timer_mutex;
static ALLEGRO_MUTEX *keybuffer_mutex;
static ALLEGRO_COND *keybuffer_cond;
static ALLEGRO_EVENT_SOURCE keybuffer_event_source;
const unsigned USER_EVENT_CLEAR_KEYBUF = ALLEGRO_GET_EVENT_TYPE('4','C','K','B');

volatile char key[KEY_MAX];
static KEYBUFFER_ENTRY keybuffer[KEYBUFFER_LENGTH];
static int keybuffer_pos;
volatile int key_shifts;

SYSTEM_DRIVER _system_driver = {0, "A5", "A5", "A5"};
SYSTEM_DRIVER *system_driver = &_system_driver;
KEYBOARD_DRIVER _keyboard_driver = {0, "A5", "A5", "A5", 0};
KEYBOARD_DRIVER *keyboard_driver = &_keyboard_driver;
MOUSE_DRIVER _mouse_driver = {0, "A5", "A5", "A5"};
MOUSE_DRIVER *mouse_driver = &_mouse_driver;
TIMER_DRIVER _timer_driver = {0, "A5", "A5", "A5"};
TIMER_DRIVER *timer_driver = &_timer_driver;
GFX_DRIVER _gfx_driver = {0, "A5", "A5", "A5", 0, 0, 0, 0, 0, 0, 0, 0};
GFX_DRIVER *gfx_driver = &_gfx_driver;
void (*keyboard_lowlevel_callback)(int scancode);
BITMAP * screen;
static A425_TLS BITMAP *current_bitmap = NULL;
static int screen_refresh_held = 0;
static FONT _font;
struct FONT * font = &_font;
int * palette_color;
ALLEGRO_DISPLAY * display;
static char window_title[128] = "";
volatile int mouse_w;
volatile int mouse_x;
volatile int mouse_y;
volatile int mouse_z;
volatile int mouse_b;
static int mickey_x, mickey_y;
int current_depth = 8;
static ALLEGRO_MOUSE_CURSOR *cursor;
static ALLEGRO_BITMAP *cursor_bitmap;
static int cursor_x, cursor_y;
struct CONFIG {
   ALLEGRO_CONFIG *allegro;
   char *name;
};
static struct CONFIG current_config;
static struct CONFIG *config_stack;
static int config_stack_size;
int _gfx_mode_set_count;
int _allegro_count;
int _allegro_in_exit = FALSE;
static int color_conversion;
void (*close_button_callback)(void);

struct{
    int draw_mode;
    enum {NORMAL, WRITE_ALPHA, MULTIPLY, TRANS} blend_mode;
    struct {int r, g, b, a;} color;
} blender;

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

#ifdef __AVX__
/* RGB byte order shifts */
static __m128i rgb_shifts;
#endif

/* forward declarations */
static void lazily_create_real_bitmap(BITMAP *bitmap, int is_mono_font);


/* allegro4 uses 0 as ok values */
static int is_ok(int code){
    if (code){
        return 0;
    }
    return -1;
}

/* returns an A5 color (ALLEGRO_COLOR) given an A4 packed int */
ALLEGRO_COLOR a5color(int a4color, int bit_depth){
    if (bit_depth == 8){
        RGB * rgb = &current_palette[a4color];
        return al_map_rgb(rgb->r * 4, rgb->g * 4, rgb->b * 4);
    }
    if (bit_depth == 16){
        return al_map_rgb(
            ((a4color >>  0) & 31) * 255 / 31,
            ((a4color >>  5) & 63) * 255 / 63,
            ((a4color >> 11) & 31) * 255 / 31);
    }
    if (bit_depth == 32){
#ifdef __AVX__
        if (a4color == MASK_COLOR_32)
            return (ALLEGRO_COLOR) { 0 };
        ALLEGRO_COLOR ret;
        a4color |= 0xFF000000;
        __m128i c_ivec = _mm_shuffle_epi8(_mm_loadu_si32(&a4color), rgb_shifts);
        __m128 c_fvec = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(c_ivec));
        __m128 c_norm = _mm_mul_ps(c_fvec, _mm_set1_ps(1.0/255.0f));
        _mm_storeu_ps((float*) &ret, c_norm);
        return ret;
#else
        return a4color != MASK_COLOR_32 ?
            al_map_rgb(getr32(a4color), getg32(a4color), getb32(a4color)) :
            (ALLEGRO_COLOR) { 0 };
#endif
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

ALLEGRO_DISPLAY* all_get_display(void){
    return display;
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
    switch (current_depth) {
    case  8: return MASK_COLOR_8;  break;
    case 15: return MASK_COLOR_15; break;
    case 16: return MASK_COLOR_16; break;
    case 24: return MASK_COLOR_24; break;
    case 32: return MASK_COLOR_32; break;
    default:
        ALLEGRO_ERROR("Unsupported current_depth depth %d\n", current_depth);
    }
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

static void set_mouse_sprite_real(ALLEGRO_BITMAP *sprite){
    if (cursor) al_destroy_mouse_cursor(cursor);
    if (sprite) {
        cursor_bitmap = sprite;
        cursor = al_create_mouse_cursor(cursor_bitmap, cursor_x, cursor_y);
        al_set_mouse_cursor(display, cursor);
        al_show_mouse_cursor(display);
    } else {
        cursor_bitmap = NULL;
        cursor = NULL;
        al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
        al_hide_mouse_cursor(display);
    }
}

void set_mouse_sprite(BITMAP *sprite){
    if (sprite) {
        lazily_create_real_bitmap(sprite, FALSE);
        set_mouse_sprite_real(sprite->real);
    } else {
        set_mouse_sprite_real(NULL);
    }
}

void set_mouse_sprite_focus(int x, int y){
    cursor_x = x;
    cursor_y = y;
    set_mouse_sprite_real(cursor_bitmap);
}

int keypressed(){
    int ret;
    al_lock_mutex(keybuffer_mutex);
    ret = (keybuffer_pos > 0);
    al_unlock_mutex(keybuffer_mutex);
    return ret;
}

int ureadkey(int *scancode){
    int ret;

    al_lock_mutex(keybuffer_mutex);
    while (keybuffer_pos == 0) {
        al_wait_cond(keybuffer_cond, keybuffer_mutex);
    }
    keybuffer_pos--;
    if (scancode) *scancode = keybuffer[keybuffer_pos].keycode;
    /* FIXME: not sure if the key_shifts should be updated here */
    key_shifts = keybuffer[keybuffer_pos].modifiers;
    ret = keybuffer[keybuffer_pos].unicode;
    al_unlock_mutex(keybuffer_mutex);
    return ret;
}

int readkey(){
    int scancode;
    int c = ureadkey(&scancode);
    return (c & 255) + (scancode << 8);
}

void clear_keybuf(){
    al_lock_mutex(keybuffer_mutex);
    {
        ALLEGRO_EVENT ev;
        ev.type = USER_EVENT_CLEAR_KEYBUF;
        al_emit_user_event(&keybuffer_event_source, &ev, NULL);
        /* Always wait for a response, even if keybuffer_pos is initially zero.
         * There may be unprocessed KEY_CHAR events already in the queue.
         */
        do {
            al_wait_cond(keybuffer_cond, keybuffer_mutex);
        } while (keybuffer_pos != 0);
    }
    al_unlock_mutex(keybuffer_mutex);
}

void simulate_ukeypress(int k, int scancode) {
    al_lock_mutex(keybuffer_mutex);
    if (keybuffer_pos < KEYBUFFER_LENGTH) {
        keybuffer[keybuffer_pos].unicode = k;
        keybuffer[keybuffer_pos].keycode = scancode;
        keybuffer[keybuffer_pos].modifiers = 0;
        keybuffer_pos++;
    }
    al_unlock_mutex(keybuffer_mutex);
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

void create_rgb_table(RGB_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))){
}

void create_light_table(COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))){
}

static void convert_8bit_inner(BITMAP * bitmap, CONVERT_8BIT mode, ALLEGRO_COLOR replace){
    if (bitmap->depth == 8){
        int x, y;
        ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bitmap->real,
                                                     ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
        unsigned char *rgba = lock->data;
        unsigned char repl[4];
        al_unmap_rgba(replace, &repl[0], &repl[1], &repl[2], &repl[3]);
        for (y = 0; y < bitmap->h; y++){
            for (x = 0; x < bitmap->w; x++){
                int c = *(bitmap->line[y] + x);
                unsigned char red, green, blue, alpha;
                switch (mode) {
                    case CONVERT_8BIT_PALETTE:
                        red = _rgb_scale_6[current_palette[c].r];
                        green = _rgb_scale_6[current_palette[c].g];
                        blue = _rgb_scale_6[current_palette[c].b];
                        alpha = 255;
                        break;
                    case CONVERT_8BIT_PALETTE_REPLACE_INDEX0:
                        if (c == 0) {
                            red = repl[0];
                            green = repl[1];
                            blue = repl[2];
                            alpha = repl[3];
                        } else {
                            red = _rgb_scale_6[current_palette[c].r];
                            green = _rgb_scale_6[current_palette[c].g];
                            blue = _rgb_scale_6[current_palette[c].b];
                            alpha = 255;
                        }
                        break;
                    case CONVERT_8BIT_INTENSITY:
                        red = green = blue = c;
                        alpha = 255;
                        break;
                    case CONVERT_8BIT_MONO_FONT:
                        if (c == 0) {
                            red = repl[0];
                            green = repl[1];
                            blue = repl[2];
                            alpha = repl[3];
                        } else {
                            red = green = blue = alpha = 255;
                        }
                        break;
		    default:
			red = green = blue = alpha = 0;
			break;
                }
                rgba[y * lock->pitch + x * 4 + 0] = red;
                rgba[y * lock->pitch + x * 4 + 1] = green;
                rgba[y * lock->pitch + x * 4 + 2] = blue;
                rgba[y * lock->pitch + x * 4 + 3] = alpha;
            }
        }
        al_unlock_bitmap(bitmap->real);
    }
}

void convert_8bit(BITMAP *bitmap, CONVERT_8BIT mode, ALLEGRO_COLOR c){
    al_destroy_bitmap(bitmap->real);
    bitmap->real = al_create_bitmap(bitmap->w, bitmap->h);
    convert_8bit_inner(bitmap, mode, c);
}

static void lazily_create_real_bitmap(BITMAP *bitmap, int is_mono_font){
    if (bitmap->real == NULL){
        CONVERT_8BIT mode;
        if (is_mono_font)
            mode = CONVERT_8BIT_MONO_FONT;
        else
            mode = CONVERT_8BIT_PALETTE_REPLACE_INDEX0;
        convert_8bit(bitmap, mode, al_map_rgba(0, 0, 0, 0));
    }
}

static void lazily_create_real_font(FONT *font){
    if (font->real) return;
    if (!font->data) {
        font->real = al_create_builtin_font();
        return;
    }
    FONT_COLOR_DATA * color_iterator;
    FONT_COLOR_DATA * color_data = font->data;
    int i, j, width = 0, height = 0;
    int maxchars = 0;
    color_iterator = color_data;
    int ranges[2 * 256];
    int ranges_count = 0;
    while (color_iterator) {
        ranges[ranges_count * 2 + 0] = color_iterator->begin;
        ranges[ranges_count * 2 + 1] = color_iterator->end - 1;
        ranges_count++;
        int count = color_iterator->end - color_iterator->begin;
        for (i = 0; i < count; i++) {
            width += color_iterator->bitmaps[i]->w;
            height = MAX(height, color_iterator->bitmaps[i]->h);
            maxchars += 1;
        }
        color_iterator = color_iterator->next;
        if (ranges_count == 256)
            break;
    }

    ALLEGRO_BITMAP *sheet = al_create_bitmap(width + maxchars + 1, height + 2);

    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_BLENDER);
    al_set_target_bitmap(sheet);
    al_clear_to_color(al_map_rgba(255, 255, 0, 0));
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

    int x = 1;
    color_iterator = color_data;
    for (j = 0; j < ranges_count; j++){
        int count = 1 + ranges[j * 2 + 1] - ranges[j * 2];
        for (i = 0; i < count; i++) {
            lazily_create_real_bitmap(color_iterator->bitmaps[i], is_mono_font(font));
            al_draw_bitmap(color_iterator->bitmaps[i]->real, x, 1, 0);
            x += color_iterator->bitmaps[i]->w + 1;
        }
        color_iterator = color_iterator->next;
    }
    al_restore_state(&state);
    font->real = al_grab_font_from_bitmap(sheet, ranges_count, ranges);
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

BITMAP * create_sub_bitmap(BITMAP *parent, int x, int y, int width, int height){
    BITMAP * bitmap = al_calloc(1, sizeof(BITMAP));
    lazily_create_real_bitmap(parent, 0);
    bitmap->real = al_create_sub_bitmap(parent->real, x, y, width, height);
    bitmap->w = width;
    bitmap->h = height;
    return bitmap;
}

static BITMAP * create_bitmap_from(ALLEGRO_BITMAP * real){
    BITMAP *bitmap = create_bitmap_ex(current_depth, al_get_bitmap_width(real),
        al_get_bitmap_height(real));
    bitmap->real = real;
    
    // TODO: other depths
    if (bitmap->depth == 16){
        ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(real,
            ALLEGRO_PIXEL_FORMAT_BGR_565, ALLEGRO_LOCK_READONLY);
        int y; for(y = 0; y < bitmap->h; y++){
            memcpy(bitmap->line[y], (char *)lock->data + y * lock->pitch,
                bitmap->w * 2);
        }
        al_unlock_bitmap(real);
    }

    if (color_conversion & COLORCONV_KEEP_TRANS) {
        al_convert_mask_to_alpha(bitmap->real, al_map_rgb(255, 0, 255));
    }
    
    return bitmap;
}

BITMAP * load_bitmap(const char * path, struct RGB *pal){
    ALLEGRO_BITMAP *bmp;
    PACKFILE *f = pack_fopen(path, F_READ);
    if (!f)
        return NULL;
    const char *ident = strrchr(path, '.');
#if ALLEGRO_VERSION_INT >= VERSION_5_1_0
    bmp = al_load_bitmap_flags_f(f, ident, ALLEGRO_NO_PREMULTIPLIED_ALPHA);
#else
    int old_flags = al_get_new_bitmap_flags();
    al_set_new_bitmap_flags(old_flags | ALLEGRO_NO_PREMULTIPLIED_ALPHA);
    bmp = al_load_bitmap_f(f, ident);
    al_set_new_bitmap_flags(old_flags);
#endif
    pack_fclose(f);
    if (bmp) {
        al_convert_mask_to_alpha(bmp, al_map_rgb(255, 0, 255));
        return create_bitmap_from(bmp);
    }
    else
	return NULL;
}

void destroy_bitmap(BITMAP* bitmap){
    if (!bitmap) return;
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
    gfx_driver->w = screen->w;
    gfx_driver->h = screen->h;
}

/* I think this method can be called after some colors are already generated.
 */
void set_color_depth(int depth){
    current_depth = depth;
}

void set_color_conversion(int mode){
    color_conversion = mode;
}

int set_gfx_mode(int card, int width, int height, int virtualwidth, int virtualheight){
    int i;
    if (display) {
        al_destroy_display(display);
        display = NULL;
    }
    if (card == GFX_TEXT)
        return 0;
    display = al_create_display(width, height);
    if (display) {
        if (window_title[0]) {
            al_set_window_title(display, window_title);
        }
        screen = create_bitmap_from(al_get_backbuffer(display));
        if (screen) {
            palette_color = palette_color8;
            set_palette(default_palette);
            setup_default_driver(screen);
            for (i = 0; i < 256; i++){
                palette_color8[i] = i;
            }
            _gfx_mode_set_count++;
        } else {
            al_destroy_display(display);
            display = NULL;
        }
        al_register_event_source(system_event_queue, al_get_display_event_source(display));

#ifdef ALLEGRO_WINDOWS
        ALLEGRO_PIXEL_FORMAT pf = al_get_display_format(display);
        switch (pf) {
        case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
    #ifdef __AVX__
           rgb_shifts = _mm_setr_epi8(2, 1, 0, 3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
    #endif
           _rgb_r_shift_32 = 16;
           _rgb_g_shift_32 = 8;
           _rgb_b_shift_32 = 0;
           _rgb_a_shift_32 = 24;
           break;
        default:
           break;
        }
#endif
        return 0;
    }
    return -1;
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

void clear(BITMAP * bitmap){
    clear_bitmap(bitmap);
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

void remove_timer() {
    int i;

    al_lock_mutex(timer_mutex);
    for (i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].callback) {
            al_stop_timer(timers[i].timer);
            timers[i].timer = NULL;
            timers[i].callback = NULL;
        }
    }
    al_unlock_mutex(timer_mutex);
}

int install_mouse(){
    if (al_install_mouse()) {
        al_register_event_source(system_event_queue,
            al_get_mouse_event_source());
        return al_get_mouse_num_buttons();
    }
    return -1;
}

int install_keyboard(){
    if (al_install_keyboard()) {
        al_register_event_source(system_event_queue, al_get_keyboard_event_source());
        al_register_event_source(system_event_queue, &keybuffer_event_source);
        return 0;
    }
    return -1;
}

static int a4key(int a5key){
    if (a5key < ALLEGRO_KEY_MODIFIERS)
        return a5key;
    switch (a5key){
        case ALLEGRO_KEY_LSHIFT: return KEY_LSHIFT;
        case ALLEGRO_KEY_RSHIFT: return KEY_RSHIFT;
        case ALLEGRO_KEY_LCTRL: return KEY_LCONTROL;
        case ALLEGRO_KEY_RCTRL: return KEY_RCONTROL;
        case ALLEGRO_KEY_ALT: return KEY_ALT;
        case ALLEGRO_KEY_ALTGR: return KEY_ALTGR;
        case ALLEGRO_KEY_LWIN: return KEY_LWIN;
        case ALLEGRO_KEY_RWIN: return KEY_RWIN;
        case ALLEGRO_KEY_MENU: return KEY_MENU;
        case ALLEGRO_KEY_SCROLLLOCK: return KEY_SCRLOCK;
        case ALLEGRO_KEY_NUMLOCK: return KEY_NUMLOCK;
        case ALLEGRO_KEY_CAPSLOCK: return KEY_CAPSLOCK;
        default: return 0;
    }
}

static int a5key(int a4key){
    if (a4key < KEY_MODIFIERS)
        return a4key;
    switch (a4key){
        case KEY_LSHIFT: return ALLEGRO_KEY_LSHIFT;
        case KEY_RSHIFT: return ALLEGRO_KEY_RSHIFT;
        case KEY_LCONTROL: return ALLEGRO_KEY_LCTRL;
        case KEY_RCONTROL: return ALLEGRO_KEY_RCTRL;
        case KEY_ALT: return ALLEGRO_KEY_ALT;
        case KEY_ALTGR: return ALLEGRO_KEY_ALTGR;
        case KEY_LWIN: return ALLEGRO_KEY_LWIN;
        case KEY_RWIN: return ALLEGRO_KEY_RWIN;
        case KEY_MENU: return ALLEGRO_KEY_MENU;
        case KEY_SCRLOCK: return ALLEGRO_KEY_SCROLLLOCK;
        case KEY_NUMLOCK: return ALLEGRO_KEY_NUMLOCK;
        case KEY_CAPSLOCK: return ALLEGRO_KEY_CAPSLOCK;
        default: return 0;
    }
}

char const *scancode_to_name(int scancode){
    return al_keycode_to_name(a5key(scancode));
}

int scancode_to_ascii(int scancode){
    return '?';
}

static void update_keyshifts(int key, int down){
    int flag;
    switch (key) {
        case ALLEGRO_KEY_LSHIFT:
        case ALLEGRO_KEY_RSHIFT:
            flag = KB_SHIFT_FLAG;
            break;
        case ALLEGRO_KEY_LCTRL:
        case ALLEGRO_KEY_RCTRL:
            flag = KB_CTRL_FLAG;
            break;
        case ALLEGRO_KEY_ALT:
        case ALLEGRO_KEY_ALTGR:
            flag = KB_ALT_FLAG;
            break;
        case ALLEGRO_KEY_LWIN:
            flag = KB_LWIN_FLAG;
            break;
        case ALLEGRO_KEY_RWIN:
            flag = KB_RWIN_FLAG;
            break;
        case ALLEGRO_KEY_MENU:
            flag = KB_MENU_FLAG;
            break;
        case ALLEGRO_KEY_COMMAND:
            flag = KB_COMMAND_FLAG;
            break;
        case ALLEGRO_KEY_SCROLLLOCK:
            flag = KB_SCROLOCK_FLAG;
            break;
        case ALLEGRO_KEY_NUMLOCK:
            flag = KB_NUMLOCK_FLAG;
            break;
        case ALLEGRO_KEY_CAPSLOCK:
            flag = KB_CAPSLOCK_FLAG;
            break;
        default:
            return;
    }
    if (down) {
        key_shifts |= flag;
    } else {
        key_shifts &=~ flag;
    }
}

static void handle_keyboard_event(ALLEGRO_EVENT *event) {
    if (event->type == ALLEGRO_EVENT_KEY_DOWN){
        int k = a4key(event->keyboard.keycode);
        key[k] = 1;
        update_keyshifts(event->keyboard.keycode, TRUE);
        if (keyboard_lowlevel_callback) {
            keyboard_lowlevel_callback(k);
        }
    } else if (event->type == ALLEGRO_EVENT_KEY_UP){
        int k = a4key(event->keyboard.keycode);
        key[k] = 0;
        update_keyshifts(event->keyboard.keycode, FALSE);
        if (keyboard_lowlevel_callback) {
            keyboard_lowlevel_callback(k + 128);
        }
    } else if (event->type == ALLEGRO_EVENT_KEY_CHAR){
        al_lock_mutex(keybuffer_mutex);
        if (keybuffer_pos < KEYBUFFER_LENGTH) {
            keybuffer[keybuffer_pos].unicode = event->keyboard.unichar;
            keybuffer[keybuffer_pos].keycode = event->keyboard.keycode;
            /* This depends on the identical correspondence of modifier
             * flags between A4 and A5.
             */
            keybuffer[keybuffer_pos].modifiers = event->keyboard.modifiers;
            keybuffer_pos++;
            al_broadcast_cond(keybuffer_cond);
        }
        al_unlock_mutex(keybuffer_mutex);
    } else if (event->type == USER_EVENT_CLEAR_KEYBUF) {
        al_lock_mutex(keybuffer_mutex);
        keybuffer_pos = 0;
        al_broadcast_cond(keybuffer_cond);
        al_unlock_mutex(keybuffer_mutex);
    }
}

static void handle_mouse_event(ALLEGRO_MOUSE_EVENT *event) {
    if (event->type == ALLEGRO_EVENT_MOUSE_AXES){
        mouse_w = event->w;
        mouse_x = event->x;
        mouse_y = event->y;
        mouse_z = event->z;
    } else if (event->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN){
        mouse_b |= 1 << (event->button - 1);
    } else if (event->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP){
        mouse_b &= ~(1 << (event->button - 1));
    }
}

static void handle_timer_event(ALLEGRO_TIMER_EVENT *event) {
    int i;

    al_lock_mutex(timer_mutex);
    for (i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].timer == event->source) {
            timers[i].callback();
            break;
        }
    }
    al_unlock_mutex(timer_mutex);
}

static void *system_thread_func(ALLEGRO_THREAD * self, void * arg){

    ALLEGRO_TIMER *retrace_timer = al_create_timer(1.0 / 60);
    al_register_event_source(system_event_queue, al_get_timer_event_source(retrace_timer));
    al_start_timer(retrace_timer);
    
    while (!al_get_thread_should_stop(self)){
        ALLEGRO_EVENT event;
        al_wait_for_event(system_event_queue, &event);
        if (event.type == ALLEGRO_EVENT_TIMER){
            if (event.timer.source == retrace_timer) {
                retrace_count++;
            } else {
                handle_timer_event(&event.timer);
            }
        }
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE){
            if (close_button_callback)
                close_button_callback();
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN ||
            event.type == ALLEGRO_EVENT_KEY_UP ||
            event.type == ALLEGRO_EVENT_KEY_CHAR ||
            event.type == USER_EVENT_CLEAR_KEYBUF)
        {
            handle_keyboard_event(&event);
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_AXES ||
            event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN ||
            event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
        {
            handle_mouse_event(&event.mouse);
        }
    }

    return NULL;
}

static bool start_system_thread(){
    system_thread = al_create_thread(system_thread_func, NULL);
#if defined(DEBUGMODE) && defined(ALLEGRO_WINDOWS)
	SetThreadDescription(*(HANDLE*)system_thread, L"Allegro 4 to 5 System Thread");
#endif
    system_event_queue = al_create_event_queue();
    timer_mutex = al_create_mutex();
    keybuffer_mutex = al_create_mutex();
    keybuffer_cond = al_create_cond();
    al_init_user_event_source(&keybuffer_event_source);
    if (system_thread && system_event_queue && timer_mutex && keybuffer_mutex && keybuffer_cond) {
        al_start_thread(system_thread);
        return true;
    }
    return false;
}

static void check_blending(){
    if (blender.draw_mode == DRAW_MODE_TRANS){
#if ALLEGRO_VERSION_INT >= VERSION_5_1_0
        if (blender.blend_mode == MULTIPLY){
            al_set_blender(ALLEGRO_ADD, ALLEGRO_DST_COLOR, ALLEGRO_ZERO);
        }
        else
#endif
        {
            al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
        }
    }
    else{
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
    }
}

static void call_constructors(void) {
   #ifndef ALLEGRO_USE_CONSTRUCTOR
      /* call constructor functions manually */
      extern void _initialize_datafile_types();

      _initialize_datafile_types();
   #endif
}

/* allegro_exit_stub:
 *  Stub function registered by the library via atexit.
 */
static void allegro_exit_stub(void)
{
    _allegro_in_exit = TRUE;

    allegro_exit();
}

int install_allegro(int system_id, int *errno_ptr, int (*atexit_ptr)(void (*func)(void))){
    return _install_allegro_version_check(system_id, errno_ptr, atexit_ptr, 0);
}

int _install_allegro_version_check(int system_id, int *errno_ptr, int (*atexit_ptr)(void (*func)(void)), int version){
    int index;

    call_constructors();

    if (!al_init())
        return -1;
    current_config.allegro = al_create_config();
    al_init_primitives_addon();
    al_init_image_addon();
    al_init_font_addon();
    if (!start_system_thread()) {
        allegro_exit();
        return -1;
    }

    for (index = 16; index < 256; index++){
        desktop_palette[index] = desktop_palette[index & 15];
    }

#ifdef __AVX__
    rgb_shifts = _mm_setr_epi8(0, 1, 2, 3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
#endif

    check_blending();

    /* install shutdown handler */
    if (_allegro_count == 0) {
        if (atexit_ptr)
            atexit_ptr(allegro_exit_stub);
    }

    _allegro_count++;

    return 0;
}

void allegro_exit(void) {
    if (system_thread) {
        /* This requires system_thread_proc to be woken up regularly,
         * such as by the retrace_count timer.
         */
        al_join_thread(system_thread, NULL);
        system_thread = NULL;
    }
    if (system_event_queue) {
        al_destroy_event_queue(system_event_queue);
        system_event_queue = NULL;
    }
    if (timer_mutex) {
        remove_timer();
        al_destroy_mutex(timer_mutex);
        timer_mutex = NULL;
    }
    if (keybuffer_mutex) {
        al_destroy_mutex(keybuffer_mutex);
        keybuffer_mutex = NULL;
    }
    if (keybuffer_cond) {
        al_destroy_cond(keybuffer_cond);
        keybuffer_cond = NULL;
    }
    al_destroy_user_event_source(&keybuffer_event_source);
    al_uninstall_system();
}

static void draw_into(BITMAP *bitmap){
	if (current_bitmap == bitmap)
        return;

    lazily_create_real_bitmap(bitmap, 0);
    if (al_is_bitmap_locked(bitmap->real))
        al_unlock_bitmap(bitmap->real);
    al_set_target_bitmap(bitmap->real);
}

void circle(BITMAP * buffer, int x, int y, int radius, int color){
    draw_into(buffer);
    al_draw_circle(x + 0.5, y + 0.5, radius, a5color(color, current_depth), 1);
}

void circlefill(BITMAP * buffer, int x, int y, int radius, int color){
    draw_into(buffer);
    al_draw_filled_circle(x + 0.5, y + 0.5, radius, a5color(color, current_depth));
}

void rect(BITMAP * buffer, int x1, int y1, int x2, int y2, int color){
    draw_into(buffer);
    al_draw_rectangle(x1 + 0.5, y1 + 0.5, x2 + 0.5, y2 + 0.5, a5color(color, current_depth), 1);
}

void rectfill(BITMAP * buffer, int x1, int y1, int x2, int y2, int color){
    draw_into(buffer);
    ALLEGRO_COLOR c = a5color(color, current_depth);
    al_draw_filled_rectangle(x1, y1, x2+1, y2+1, c);
}

void triangle(BITMAP * buffer, int x1, int y1, int x2, int y2, int x3, int y3, int color){
    draw_into(buffer);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, a5color(color, current_depth));
}

int getpixel(BITMAP * buffer, int x, int y){
    lazily_create_real_bitmap(buffer, 0);
    ALLEGRO_BITMAP * al_buffer = buffer->real;
    if (!al_is_bitmap_locked(al_buffer)){
        al_lock_bitmap(buffer->real, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
    }
    return a4color(al_get_pixel(al_buffer, x, y), current_depth);
}

void putpixel(BITMAP * buffer, int x, int y, int color){
    lazily_create_real_bitmap(buffer, 0);
    if (blender.draw_mode == DRAW_MODE_TRANS && blender.blend_mode == WRITE_ALPHA){
        if (!al_is_bitmap_locked(buffer->real)){
            al_lock_bitmap(buffer->real, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
        }
        al_set_target_bitmap(buffer->real);
        ALLEGRO_COLOR c = al_get_pixel(buffer->real, x, y);
        c.a = color / 255.0;
        al_put_pixel(x, y, c);
    } else {
        if (al_is_bitmap_locked(buffer->real)){
            al_set_target_bitmap(buffer->real);
            al_put_pixel(x, y, a5color(color, current_depth));
        }
        else {
            draw_into(buffer);
            al_draw_pixel(x + 0.5, y + 0.5, a5color(color, current_depth));
        }
    }
}

void line(BITMAP * buffer, int x, int y, int x2, int y2, int color){
    draw_into(buffer);
    al_draw_line(x + 0.5, y + 0.5, x2 + 0.5, y2 + 0.5, a5color(color, current_depth), 1);
}

void hline(BITMAP * buffer, int x, int y, int x2, int color){
    draw_into(buffer);
    al_draw_line(x, y + 0.5, x2 + 1, y + 0.5, a5color(color, current_depth), 1);
}

void vline(BITMAP * buffer, int x, int y, int y2, int color){
    draw_into(buffer);
    al_draw_line(x + 0.5, y, x + 0.5, y2 + 1, a5color(color, current_depth), 1);
}

void hold_screen_refresh(int hold) {
    if (hold) {
	if (screen_refresh_held == 0)
	    screen_refresh_held = 1;
    } else {
	if (screen_refresh_held > 1)
	    al_flip_display();
	screen_refresh_held = 0;
    }
}

static void maybe_flip_screen(BITMAP * where){
    if (where == screen){
	if (screen_refresh_held == 0) {
	    al_flip_display();
	} else {
	    screen_refresh_held++;
	}
    }
}

void stretch_blit(BITMAP *source, BITMAP *dest, int source_x,
    int source_y, int source_width, int source_height, int dest_x,
    int dest_y, int dest_width, int dest_height){

    lazily_create_real_bitmap(source, 0);
    lazily_create_real_bitmap(dest, 0);

    ALLEGRO_BITMAP * al_from = source->real;
    ALLEGRO_BITMAP * al_to = dest->real;

    if (al_is_bitmap_locked(al_from))
        al_unlock_bitmap(al_from);

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
        if (blender.draw_mode == DRAW_MODE_TRANS){
            ALLEGRO_COLOR tint = al_map_rgba(blender.color.r,
                blender.color.g, blender.color.b, blender.color.a);
            al_draw_tinted_scaled_bitmap(al_from, tint, source_x, source_y,
                source_width,
                source_height, dest_x, dest_y, dest_width, dest_height, 0);
        } else {
            al_draw_scaled_bitmap(al_from, source_x, source_y,
                source_width,
                source_height, dest_x, dest_y, dest_width, dest_height, 0);
        }
    }

    maybe_flip_screen(dest);
}

void blit(BITMAP * from, BITMAP * to, int from_x, int from_y, int to_x, int to_y, int width, int height){
    stretch_blit(from, to, from_x, from_y, width, height, to_x, to_y, width, height);
}

void draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y){
    blit(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h);
}

void stretch_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int w, int h){
    stretch_blit(sprite, bmp, 0, 0, sprite->w, sprite->h, x, y, w, h);
}

void masked_blit(BITMAP * from, BITMAP * to, int from_x, int from_y, int to_x, int to_y, int width, int height){
    blit(from, to, from_x, from_y, to_x, to_y, width, height);
}

void draw_trans_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y){
    draw_into(bmp);
    lazily_create_real_bitmap(sprite, 0);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
    ALLEGRO_COLOR tint = al_map_rgba(blender.color.a,
        blender.color.a, blender.color.a, blender.color.a);
    al_draw_tinted_bitmap(sprite->real, tint, x, y, 0);
}

void draw_sprite_h_flip(BITMAP *bmp, BITMAP *sprite, int x, int y){
    draw_into(bmp);
    lazily_create_real_bitmap(sprite, 0);
    al_draw_bitmap(sprite->real, x, y, ALLEGRO_FLIP_HORIZONTAL);
}

void draw_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y){
    draw_into(bmp);
    lazily_create_real_bitmap(sprite, 0);
    al_draw_bitmap(sprite->real, x, y, ALLEGRO_FLIP_VERTICAL);
}

void draw_sprite_vh_flip(BITMAP *bmp, BITMAP *sprite, int x, int y){
    draw_into(bmp);
    lazily_create_real_bitmap(sprite, 0);
    al_draw_bitmap(sprite->real, x, y, ALLEGRO_FLIP_HORIZONTAL | ALLEGRO_FLIP_VERTICAL);
}

static float radians(fixed angle){
    return fixtof(angle) * ALLEGRO_PI / 128.0;
}

void pivot_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle){
    draw_into(bmp);
    lazily_create_real_bitmap(sprite, 0);
    al_draw_rotated_bitmap(sprite->real, cx, cy, x, y, radians(angle), 0);
}

void pivot_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle){
    draw_into(bmp);
    lazily_create_real_bitmap(sprite, 0);
    al_draw_rotated_bitmap(sprite->real, cx, cy, x, y, radians(angle), ALLEGRO_FLIP_VERTICAL);
}

void draw_lit_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int a){
    lazily_create_real_bitmap(sprite, 0);
    ALLEGRO_BITMAP *temp = al_create_bitmap(sprite->w, sprite->h);
    ALLEGRO_COLOR tint = al_map_rgba(blender.color.r,
        blender.color.g, blender.color.b, 255);
    al_set_target_bitmap(temp);
    al_clear_to_color(tint);
    al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE,
        ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
    float f = 1 - a / 255.0;
    ALLEGRO_COLOR alpha = {f, f, f, 1};
    al_draw_tinted_bitmap(sprite->real, alpha, 0, 0, 0);
    draw_into(bmp);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
    al_draw_bitmap(temp, x, y, 0);
    al_destroy_bitmap(temp);
}

static ALLEGRO_COLOR text_color(int color, int current_depth){
    if (color == -1) {
        return al_map_rgb(255, 255, 255);
    } else {
        return a5color(color, current_depth);
    }
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
    al_draw_text(f->real, text_color(color, current_depth), x, y, ALLEGRO_ALIGN_CENTRE, str);
    maybe_flip_screen(bmp);
}

void textout_right_ex(struct BITMAP *bmp, struct FONT *f, AL_CONST char *str, int x, int y, int color, int bg){
    lazily_create_real_font(f);
    al_set_target_bitmap(bmp->real);
    if (bg != -1) {
        int w = al_get_text_width(f->real, str);
        al_draw_filled_rectangle(x - w / 2, y, x + w - w / 2,
            y + al_get_font_line_height(f->real), a5color(bg, current_depth));
    }
    al_draw_text(f->real, text_color(color, current_depth), x, y, ALLEGRO_ALIGN_RIGHT, str);
    maybe_flip_screen(bmp);
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
    al_draw_text(f->real, text_color(color, current_depth), x, y, 0, str);

    maybe_flip_screen(bmp);
}

int _textmode = 0;

int text_mode(int mode)
{
   int old_mode = _textmode;

   if (mode < 0)
      _textmode = -1;
   else
      _textmode = mode;

   return old_mode;
}

void textprintf_centre(BITMAP *bmp, FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(bmp);
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout_centre_ex(bmp, f, buf, x, y, color, _textmode);
}

void textprintf_right(BITMAP *bmp, FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(bmp);
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout_right_ex(bmp, f, buf, x, y, color, _textmode);
}

void textprintf(BITMAP *bmp, FONT *f, int x, int y, int color, AL_CONST char *format, ...)
{
   char buf[512];
   va_list ap;
   ASSERT(bmp);
   ASSERT(f);
   ASSERT(format);

   va_start(ap, format);
   uvszprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   textout_ex(bmp, f, buf, x, y, color, _textmode);
}

void textout(struct BITMAP *bmp, FONT *f, AL_CONST char *str, int x, int y, int color)
{
   textout_ex(bmp, f, str, x, y, color, _textmode);
}

void textout_centre(struct BITMAP *bmp, FONT *f, AL_CONST char *str, int x, int y, int color)
{
   textout_centre_ex(bmp, f, str, x, y, color, _textmode);
}

void textout_right(struct BITMAP *bmp, FONT *f, AL_CONST char *str, int x, int y, int color)
{
   textout_right_ex(bmp, f, str, x, y, color, _textmode);
}

void draw_gouraud_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4){
    draw_into(bmp);
    al_draw_bitmap(sprite->real, x, y, 0);
}

void set_clip_rect(BITMAP * bitmap, int x1, int y1, int x2, int y2){
    lazily_create_real_bitmap(bitmap, 0);
    al_set_target_bitmap(bitmap->real);
    al_set_clipping_rectangle(x1, y1, x2 - x1, y2 - y1);
}

void set_clip_state(BITMAP * bitmap, int onoff){

}

void set_clip(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
   if ((!x1) && (!y1) && (!x2) && (!y2)) {
      set_clip_rect(bitmap, 0, 0, bitmap->w-1, bitmap->h-1);
      set_clip_state(bitmap, FALSE);
      return;
   }
   set_clip_rect(bitmap, x1, y1, x2, y2);
   set_clip_state(bitmap, TRUE);
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
    /* Color 0 (MASK_COLOR_8) is special in 8-bit mode. It is used with custom pointer in exmouse.
     * We cannot move this logic to a5color as palette[0] is valid and is usually black.
     * E.g. see exstars/erase_ship().
     * For 32-bit mode, it is expected to do that in a5color.
     * The Allegro GUI Un-uglification Project (AGUP) uses putpixel with MASK_COLOR_32. */
    if (color || current_depth != 8)
        al_clear_to_color(a5color(color, current_depth));
    else
        al_clear_to_color((ALLEGRO_COLOR) { 0 });
}

void acquire_screen(){
    draw_into(screen);
}

void release_screen(){
    maybe_flip_screen(screen);
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

void polygon(BITMAP *bmp, int vertices, AL_CONST int *points, int color) {
    int limit = vertices << 1;
    float *pts = al_malloc(limit * sizeof(float));
    int i = 0;
#ifdef __AVX__
	/* process 4 vertices at a time but make sure we stay within array bounds */
    limit -= 6;
    for (; i < limit; i += 8) {
        __m256i int_vec = _mm256_loadu_si256((const __m256i *)(points + i));
        __m256 float_vec = _mm256_cvtepi32_ps(int_vec);
        _mm256_storeu_ps(pts + i, float_vec);
    }
    /* then process the rest */
    limit += 6;
#endif
	for (; i < limit; i++) {
		pts[i] = points[i];
	}
	draw_into(bmp);
    /* We don't check polygon orientation here. It is important in A5! */
	al_draw_filled_polygon(pts, vertices, a5color(color, current_depth));
    al_free(pts);
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

int install_int_ex(void (*proc)(void), long speed){
    double fspeed = (double) speed / (double) TIMERS_PER_SECOND;
    int ret = -1;
    int i;

    al_lock_mutex(timer_mutex);

    /* Change speed if proc already exists. */
    for (i = 0; i < MAX_TIMERS; i++) {
        if (proc == timers[i].callback) {
            al_set_timer_speed(timers[i].timer, fspeed);
            ret = 0;
            break;
        }
    }

    /* Add new timer if proc wasn't found. */
    if (i == MAX_TIMERS) {
        for (i = 0; i < MAX_TIMERS; i++) {
            if (timers[i].timer == NULL) {
                timers[i].timer = al_create_timer(fspeed);
                if (timers[i].timer != NULL) {
                    timers[i].callback = proc;
                    al_register_event_source(system_event_queue,
                        al_get_timer_event_source(timers[i].timer));
                    al_start_timer(timers[i].timer);
                    ret = 0;
                }
                break;
            }
        }
    }

    al_unlock_mutex(timer_mutex);

    return ret;
}

int install_int(void (*proc)(void), long speed){
    return install_int_ex(proc, MSEC_TO_TIMER(speed));
}

void remove_int(void (*proc)(void)){
    int i;

    al_lock_mutex(timer_mutex);
    for (i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].callback == proc) {
            al_stop_timer(timers[i].timer);
            timers[i].timer = NULL;
            timers[i].callback = NULL;
        }
    }
    al_unlock_mutex(timer_mutex);
}

void push_config_state(){
    config_stack_size++;
    config_stack = al_realloc(config_stack, config_stack_size * sizeof *config_stack);
    config_stack[config_stack_size - 1] = current_config;
    current_config.allegro = al_create_config();
}

void pop_config_state(){
    if (current_config.allegro) al_destroy_config(current_config.allegro);
    current_config = config_stack[config_stack_size - 1];
    config_stack_size--;
    config_stack = al_realloc(config_stack, config_stack_size * sizeof *config_stack);
}

void override_config_file(AL_CONST char *filename)
{
   set_config_file(filename);
}

void override_config_data(AL_CONST char *data, int length)
{
   ALLEGRO_WARN("Not implemented\n");
}

void set_config_file(char const *filename){
    if (current_config.allegro) al_destroy_config(current_config.allegro);
    current_config.allegro = al_load_config_file(filename);
    if (!current_config.allegro) current_config.allegro = al_create_config();
   if (current_config.name) free(current_config.name);
   current_config.name = strdup(filename);
}

void set_config_string(AL_CONST char *section, AL_CONST char *name, AL_CONST char *val)
{
    if (!current_config.allegro)
        return;
   if (val)
      al_set_config_value(current_config.allegro, section, name, val);
   else
      al_remove_config_key(current_config.allegro, section, name);
   if (current_config.name)
      al_save_config_file(current_config.name, current_config.allegro);
}

static char _tokens[64][64];
static char *tokens[64];
char **get_config_argv(char const *section, char const *name, int *argc){
    char const *v;
    int i, pos = 0;
    v = al_get_config_value(current_config.allegro, section, name);
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
    char const *v = al_get_config_value(current_config.allegro, section, name);
    if (!v) return def;
    return v;
}

int get_config_int(char const *section, char const *name, int def){
    char const *v = al_get_config_value(current_config.allegro, section, name);
    if (!v) return def;
    return (int)strtol(v, NULL, 10);
}

/* get_config_hex:
 *  Reads a hexadecimal integer from the configuration file.
 */
int get_config_hex(AL_CONST char *section, AL_CONST char *name, int def)
{
    AL_CONST char *s = get_config_string(section, name, NULL);
    char tmp[64];
    long i;

    if ((s) && (ugetc(s))) {
        i = ustrtol(s, NULL, 16);
        if ((i == 0x7FFFFFFF) && (ustricmp(s, uconvert_ascii("7FFFFFFF", tmp)) != 0))
            i = -1;
        return i;
    }

    return def;
}

/* add_unique_name
 *  Helper to add a name to a list of names.
 */
static int add_unique_name(AL_CONST char ***names, int n, char const *name)
{
    int i;
    /* FIXME: use better search algorithm */
    for (i = 0; i < n; i++)
        if (!ustrcmp((*names)[i], name))
            return n;

    *names = _al_sane_realloc((void *)*names, (n + 1) * sizeof * *names);
    (*names)[n] = name;
    return n + 1;
}

/* list_config_entires:
 *  Returns the names of all config entries in a section. The names parameter is
 *  a pointer to a strings array that will contain the config keys. If it points to
 *  a NULL pointer, it will be allocated, or else re-allocated accordingly. The
 *  return value tells how many valid string pointers it contains after the
 *  function returns.
 */
int list_config_entries(AL_CONST char *section, AL_CONST char ***names)
{
    int n = 0;
    ALLEGRO_CONFIG_SECTION *section_iterator = NULL;
    const char *section_name = al_get_first_config_section(current_config.allegro, &section_iterator);
    while (section_name) {
        if (ustricmp(section_name, section) == 0) {
            ALLEGRO_CONFIG_ENTRY *entry_iterator = NULL;
            const char *key = al_get_first_config_entry(current_config.allegro, section_name, &entry_iterator);
            while (key) {
                n = add_unique_name(names, n, key);
                key = al_get_next_config_entry(&entry_iterator);
            }
            break;
        }
        section_name = al_get_next_config_section(&section_iterator);
    }
    return n;
}

/* free_config_entries:
 *  Frees the entries list returned by list_config_entires or
 *  list_config_sections again.
 */
void free_config_entries(AL_CONST char ***names)
{
    al_free(*names);
    *names = NULL;
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
    bool ok = al_save_bitmap(filename, bmp->real);
    (void)pal;
    return is_ok(ok);
}

int is_video_bitmap(BITMAP * what){
    /* All bitmaps are video */
    return 1;
}

void rotate_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle){
    pivot_sprite(bmp, sprite, x + sprite->w/2, y + sprite->h/2, sprite->w/2, sprite->h/2, angle);
}

void rotate_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle) {
    pivot_sprite_v_flip(bmp, sprite, x + sprite->w/2, y + sprite->h/2, sprite->w/2, sprite->h/2, angle);
}

void rotate_scaled_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale){
    /* FIXME */
}

void rotate_scaled_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale){
    /* FIXME */
}

void quad3d(struct BITMAP *bmp, int type, struct BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4){
    /* FIXME */
}

void drawing_mode(int mode, struct BITMAP *pattern, int x_anchor, int y_anchor){
    blender.draw_mode = mode;
    check_blending();
}

void solid_mode(){
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
}

void set_write_alpha_blender(void){
    blender.blend_mode =  WRITE_ALPHA;
    check_blending();
}

void set_multiply_blender(int r, int g, int b, int a){
    blender.blend_mode = MULTIPLY;
    blender.color.r = r;
    blender.color.g = g;
    blender.color.b = b;
    blender.color.a = a;
    check_blending();
}

void set_trans_blender(int r, int g, int b, int a){
    blender.blend_mode = TRANS;
    blender.color.r = r;
    blender.color.g = g;
    blender.color.b = b;
    blender.color.a = a;
    check_blending();
}

void set_alpha_blender(void){
    blender.blend_mode = NORMAL;
    check_blending();
}

int show_video_bitmap(BITMAP *bitmap){
    blit(bitmap, screen, 0, 0, 0, 0, bitmap->w, bitmap->h);
    return 0;
}

int text_length(AL_CONST struct FONT *f, AL_CONST char *str){
    lazily_create_real_font((FONT *)f);
    return al_get_text_width(f->real, str);
}

int text_height(AL_CONST struct FONT *f){
    lazily_create_real_font((FONT *)f);
    return al_get_font_line_height(f->real);
}

void set_keyboard_rate(int delay, int repeat){
    /* FIXME */
}

void set_config_int(AL_CONST char *section, AL_CONST char *name, int val){
   char sval[1024];
   snprintf(sval, sizeof sval, "%d", val);
   set_config_string(section, name, sval);
}

/* set_config_hex:
 *  Writes a hexadecimal integer to the configuration file.
 */
void set_config_hex(AL_CONST char *section, AL_CONST char *name, int val)
{
    char buf[32], tmp[32];

    if (val >= 0) {
        uszprintf(buf, sizeof(buf), uconvert_ascii("%X", tmp), val);
        set_config_string(section, name, buf);
    }
    else
        set_config_string(section, name, uconvert_ascii("-1", buf));
}

int show_os_cursor(int cursor){
    /* FIXME */
    return -1;
}

int desktop_color_depth(void){
    /* FIXME */
    return 32;
}

// MINE
int get_color_depth(){
    return current_depth;
}

int get_desktop_resolution(int *width, int *height) {
    ALLEGRO_MONITOR_INFO info;
    al_get_monitor_info(0, &info);
    *width = info.x2 - info.x1;
    *height = info.y2 - info.y1;
    return 0;
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


int set_close_button_callback(void (*proc)(void)){
   close_button_callback = proc;
    return 0;
}

void set_window_title(AL_CONST char *name){
    _al_sane_strncpy(window_title, name, sizeof(window_title));
    if (display)
        al_set_window_title(display, window_title);
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

//void *_al_sane_realloc(void *ptr, size_t size){
//    return al_realloc(ptr, size);
//}

int set_display_switch_callback(int dir, void (*cb)()){
    return 0;
}

int get_display_switch_mode(){
    return SWITCH_BACKGROUND;
}

void remove_display_switch_callback(void (*cb)()){
}
// Keeps track of the current drawing target
static ALLEGRO_BITMAP *selected_bitmap = NULL;

//Equivalent to Allegro 4's bmp_select()
void bmp_select(BITMAP *bitmap){
    if (bitmap && bitmap->real) {
        selected_bitmap = bitmap->real;
        al_set_target_bitmap(selected_bitmap);
    } else {
        selected_bitmap = NULL;
        ALLEGRO_DISPLAY *display = al_get_current_display();
        if (display)
            al_set_target_backbuffer(display);
    }
}

uintptr_t bmp_read_line(BITMAP *bitmap, int row){
    return (uintptr_t)bitmap->line[row];
}

uintptr_t bmp_write_line(BITMAP *bitmap, int row){
    al_lock_bitmap_region(bitmap->real, 0, row, bitmap->w, 1,
        ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
    return (uintptr_t)bitmap->line[row];
}

void bmp_unwrite_line(BITMAP *bitmap){
    al_unlock_bitmap(bitmap->real);
}
   
void ellipsefill(BITMAP * buffer, int x, int y, int rx, int ry, int color){
    draw_into(buffer);
    al_draw_filled_ellipse(x, y, rx, ry, a5color(color, current_depth));
}

static double fixangle_to_radians(fixed angle){
    return (fixtof(angle) * 360 / 256.0) * AL_PI / 180;
}

void arc(BITMAP * buffer, int x, int y, fixed ang1, fixed ang2, int r, int color){
    draw_into(buffer);
    al_draw_arc(x, y, r, fixangle_to_radians(ang1), fixangle_to_radians(ang2), a5color(color, current_depth), 1);
}

#ifdef ALLEGRO_WINDOWS
/* _al_drive_exists:
 *  Checks whether the specified drive is valid.
 */
int _al_drive_exists(int drive)
{
   return GetLogicalDrives() & (1 << drive);
}

/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive(void)
{
    return _getdrive() - 1;
}
#endif
