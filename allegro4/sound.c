#include "allegro.h"
#include "include/internal/aintern.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

static int digi_reserve = -1;             /* how many voices to reserve */

/* allegro4 uses 0 as ok values */
static int is_ok(int code){
    if (code){
        return 0;
    }
    return -1;
}

void reserve_voices(int digi_voices, int midi_voices){
    digi_reserve = digi_voices;
}

int install_sound(int digi, int midi, AL_CONST char *cfg_path){
    if (al_install_audio() && al_init_acodec_addon()) {
        if (digi_reserve >= 0)
            al_reserve_samples(digi_reserve);
        else
            al_reserve_samples(MIXER_DEF_SFX);
        return 0;
    } else {
        return -1;
    }
}

void release_voice(int voice){
    /* FIXME */
}

static void lazily_create_sample(SAMPLE * sample){
    if (sample->real == NULL){
        ALLEGRO_CHANNEL_CONF channels = ALLEGRO_CHANNEL_CONF_1;
        ALLEGRO_AUDIO_DEPTH depth = ALLEGRO_AUDIO_DEPTH_UINT8;
        switch (sample->stereo){
            case 0: channels = ALLEGRO_CHANNEL_CONF_1; break;
            case 1: channels = ALLEGRO_CHANNEL_CONF_2; break;
        }
        switch (sample->bits){
            case 8: depth = ALLEGRO_AUDIO_DEPTH_UINT8; break;
            case 16: depth = ALLEGRO_AUDIO_DEPTH_UINT16; break;
        }
        sample->real = al_create_sample(sample->data, sample->len, sample->freq, depth, channels, false);
    }
}

int play_sample(AL_CONST SAMPLE * sample, int volume, int pan, int frequency, int loop){
    int a5_loop = ALLEGRO_PLAYMODE_ONCE;
    lazily_create_sample((SAMPLE*) sample);
    switch (loop){
        case 1: a5_loop = ALLEGRO_PLAYMODE_LOOP; break;
        default: a5_loop = ALLEGRO_PLAYMODE_ONCE; break;
    }
    return is_ok(al_play_sample(sample->real, volume / 255.0, (pan - 128.0) / 128.0, frequency / 1000.0, a5_loop, NULL));
}

SAMPLE *load_sample(AL_CONST char *filename)
{
    SAMPLE *sample = al_calloc(sizeof *sample, 1);
    sample->real = al_load_sample(filename);
    switch (al_get_sample_depth(sample->real)) {
        case ALLEGRO_AUDIO_DEPTH_UINT8: sample->bits = 8; break;
        case ALLEGRO_AUDIO_DEPTH_UINT16: sample->bits = 16; break;
        default: break;
    }
    switch (al_get_sample_channels(sample->real)) {
        case ALLEGRO_CHANNEL_CONF_2: sample->stereo = 1; break;
        default: break;
    }
    return sample;
}

void destroy_sample(SAMPLE *sample){
    if (!sample) return;

    if (sample->real) {
        al_destroy_sample(sample->real);
    }
    al_free(sample);
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
