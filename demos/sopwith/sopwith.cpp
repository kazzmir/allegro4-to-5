/* Sopwith by Jon Rafkind
 * Originally created sometime around 1998-1999
 */

#include <allegro.h>
#include "trigtable.h"
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/***********************PROTOTYPES*******(if any)****************************/

void change_dx();
void make_plane_explosion( int mx, int my );
void make_explosion( int x, int y, double walkx, double walky, int rad );
void re_init_player();
void kill_player();
void minor_init();

/***********************STRUCTURES*******************************************/

enum build_type {

        base = 55,
        cow = 15,
        gun = 20,
        office = 56,
        windmill = 30,
        plant = 35,
        oil_tower = 60

};

struct explosion_rec {

        int x, y;
        double vx, vy;
        double wx, wy;
        int radius;
        int max_rad;
        bool alive;

};

struct bullet_rec {

        int x, y;
        double vx, vy;
        double wx, wy;
        bool fall;
        int life;

};

struct plane_rec {

        bool stall;
        int showx, showy;
        int realx, realy;
        double virtx, virty;
        double speed;
        double dx, dy;

        int bomb_counter;
        int shot_counter;
        int flip_counter;

        int fuel_left;
        int bombs_left;
        int shots_left;

        int dead_phase;
        int angle;
        int allegiance;
        int health;
        bool landed;

};

struct bomb_rec {

        int x, y;
        double vx, vy;
        double wx, wy;
        bool alive;
        int mirv;
        int mirv_life;

};

struct player_rec {

        struct plane_rec my_plane;
        bool flip;
        bool home;
        int home_ang;
        int shooting_ang;
        bool shoot_up;
        long int score;
        long int real_score;
        long int next_level;
        int mission;
        int lives;

        double shx, shy;

} player;

struct enemy_rec {

        struct plane_rec my_plane;
        bool flip;
        bool alive;
        int health;
        int from;
        int want_ang;
        int go_up;

};

class buildingClass {
        public:
                buildingClass(){}

                bool direction;
                bool alive;
                bool rubble;
                int phase;
                int x, y;
                int size;
                int width;
                enum build_type kind;
                int allegiance;
                int health;
                int has_plane;
                int max_plane, make_plane;

};

struct building_rec {

        bool direction;
        bool alive;
        bool rubble;
        int phase;
        int x, y;
        int size;
        int width;
        enum build_type kind;
        int allegiance;
        int health;
        int has_plane;
        int max_plane, make_plane;

};

struct mission_rec {

        int starting_lives;
        double max_plane_speed;
        double min_plane_speed;
        double max_player_speed;
        int min_enemy_int;
        int enemy_int;
        int enemy_bases;
        int other_buildings;

};

/***********************VARIABLES********************************************/

/* I was way into pascal at the time */
#define repeat do {
#define until( y ) } while ( !y );

int rest_time = 0;
int lastfps = 80;

#define max_health 50
#define max_fuel 15000
#define base_length 250
#define gen_plane_length 10
#define minimum 150
#define land_color 241
#define max_land_color 14
#define dark_land_color 251
#define expl_color 240
#define max_land 3500
#define max_land_height 300
#define max_buildings 50
#define max_expl 100
#define max_bomb 20
#define max_bomb_left 12
#define max_shot_left 100
#define max_shot 200
#define max_enemy 10
#define max_missions 15
#define bullet_speed 1.56
//-------------------------sound stuff---------------------
#define max_sample 10
#define snd_death 0
#define snd_hurt_1 1
#define snd_hurt_2 2
#define snd_hurt_3 3
#define snd_destroy 4
#define snd_shoot 5
#define snd_bomb_away 6
#define snd_bomb_hit 7
#define snd_plane_crash 8
#define snd_theme_song 9
//-------------------------end sound stuff-----------------
#define angular_1 135
#define angular_2 180

#ifdef PANDORA
#define screen_x 800
#else
#define screen_x 640
#endif
#define screen_y 480

#define GAME_SPEED 19

PALLETE original;
SAMPLE * snd[ max_sample ];
struct mission_rec mission[ max_missions ];
struct enemy_rec enemy[ max_enemy ];
struct bullet_rec shot[ max_shot ];               //hmm..
struct explosion_rec explr[ max_expl ];           //.....
struct bomb_rec bomb[ max_bomb ];                 //.....
struct building_rec build[ max_buildings ];       //..okay
int land[ max_land ];                             //the land that you see
int camera = 0;                                   //camera to tell where to look
int ultra_color = 0;

#ifdef PANDORA
int K_ACCELERATE = KEY_RIGHT;
int K_DECCELERATE = KEY_LEFT;
int K_TURNLEFT = KEY_UP;
int K_TURNRIGHT = KEY_DOWN;
int K_SHOOT = KEY_HOME;      // A
int K_CANNON = KEY_DEL;      // Del
int K_MULTIBOMB = KEY_END;   // B
int K_NORMALBOMB = KEY_PGDN; // X
int K_SHOTBOMB = KEY_PGUP;   // Y
int K_FLIP = KEY_RSHIFT;     // L
int K_HOME = KEY_H;
int K_QUIT = KEY_Q;
int K_EDIT = KEY_E;
int K_SCREENSHOT = KEY_U;
#else
int K_SHOOT = KEY_SPACE;
int K_CANNON = KEY_A;
int K_MULTIBOMB = KEY_V;
int K_NORMALBOMB = KEY_B;
int K_SHOTBOMB = KEY_N;
int K_TURNLEFT = KEY_COMMA;
int K_TURNRIGHT = KEY_SLASH;
int K_FLIP = KEY_STOP;
int K_HOME = KEY_H;
int K_ACCELERATE = KEY_X;
int K_DECCELERATE = KEY_C;
int K_QUIT = KEY_Q;
int K_EDIT = KEY_E;
int K_SCREENSHOT = KEY_U;
#endif

unsigned int grand_clock = 0;

int map_color = 31;
volatile int speed_counter;

BITMAP *work;
BITMAP *land_work;
DATAFILE * data_snd;

/****************************************************************************/

void YIELD(){
    rest(1);
    /*
	#ifdef UNIX
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1;
	select( 0, NULL, NULL, NULL, &tv );
	#else
	//sleep(1);
	yield_timeslice();
	#endif
        */
}

int get_width( enum build_type what ) {

        int r = what;

        switch ( r ) {

                case base   :  return 15; break;
                case cow    :  return 20; break;
                case plant  :  return 40; break;
                case gun    :  return 25; break;
                case windmill  :  return 35; break;
                case oil_tower :  return 25; break;
                case office    :  return 35; break;
        }

        return 0;
}


bool not_hit_something( int x1, int y1, int xl, int yl ) {

        int x2 = x1 + xl;
        int y2 = y1 + yl;

        for ( int r = x1; r <= x2; r++ ) {

                if ( y2 >= screen_y - land[ r ] )
                        return false;

        }
        return true;

}


char *int2str( long int e ) {

        long int h = e;
        bool sign = false;
        if ( h < 0 ) {
                h = -h;
                sign = true;
        }
        char *mine = new char[100];
        for ( int r = 0; r< 100; r++ )
                mine[ r ] = '0';
        int length = 0;

        if ( sign ) {

                mine[0] = '-';

        }

        while ( h > 0 ) {
                length++;
                int gr = sign;
                for ( int r = gr; r< length+gr; r++ )
                        mine[ length+gr-r ] = mine[ length+gr-r-1 ];

                mine[sign] = (char)( h % 10 + (int)'0' );
                h = h / 10;

        }

        if ( length == 0 )
                length = 1;
        char *rep = new char[ length+sign ];
        for ( int r = 0; r< length+sign; r++ )
                rep[r] = mine[r];
        rep[ length+sign ] = '\0';

        delete[] mine;

        return rep;

}


double random_float( int w ) {

        double total = (double)(random()%w)+(double)(random()%10)/10.0+(double)(random()%10)/100.0+(double)(random()%10)/1000.0;
        return total;

}


                                                  /*, int *ix, int *iy*/
int line_hit( int ax, int ay, int bx, int by, int cx, int cy, int dx, int dy  ) {
        float r, s;
        float denom = (bx-ax)*(dy-cy)-(by-ay)*(dx-cx);

        r = (float)((ay-cy)*(dx-cx)-(ax-cx)*(dy-cy))/denom;
        s = (float)((ay-cy)*(bx-ax)-(ax-cx)*(by-ay))/denom;

        if ( r < 0.0 || s < 0.0 || r > 1.0 || s > 1.0 )
                return 0;

        /*
         *ix=ax+(int)(r*(float)(bx-ax)+0.5);
         *iy=ay+(int)(r*(float)(by-ay)+0.5);
         */

        return 1;

}                                                 // end lineIntersect


int posneg() {
        int r;
        r = random() % 2;
        if ( r == 0 )
                return 1;
        else
                return -1;
}


bool point_in_triangle( int x1, int y1, int x2, int y2, int x3, int y3, int mx, int my ) {

        int ang = random() % 360;

        int mx2, my2;
        mx2 = (int)(mx + tcos[ ang ] * 15000 );
        my2 = (int)(my + tsine[ ang ] * 15000 );

        int count = 0;
        if ( line_hit( x1, y1, x2, y2, mx, my, mx2, my2 ) &&
                line_hit( mx, my, mx2, my2, x1, y1, x2, y2 ) )
                count++;
        if ( line_hit( x2, y2, x3, y3, mx, my, mx2, my2 ) &&
                line_hit( mx, my, mx2, my2, x2, y2, x3, y3 ) )
                count++;
        if ( line_hit( x1, y1, x3, y3, mx, my, mx2, my2 ) &&
                line_hit( mx, my, mx2, my2, x1, y1, x3, y3 ) )
                count++;

        if ( count == 1 || count == 3 ) {

                ang = ( ang + 180 ) % 360;
                mx2 = (int)(mx + tcos[ ang ] * 15000 );
                my2 = (int)(my + tsine[ ang ] * 15000 );

                count = 0;
                if ( line_hit( x1, y1, x2, y2, mx, my, mx2, my2 ) )
                        count++;
                if ( line_hit( x2, y2, x3, y3, mx, my, mx2, my2 ) )
                        count++;
                if ( line_hit( x1, y1, x3, y3, mx, my, mx2, my2 ) )
                        count++;

                if ( count == 1 || count == 3 )
                        return true;
                else
                        return false;

        }
        else
                return false;

}


void get_triangle_points( int &x1, int &y1, int &x2, int &y2, int &x3, int &y3 ) {

        int mx = player.my_plane.realx;
        int my = player.my_plane.realy;

        int ang = player.my_plane.angle;

        int plane_length = gen_plane_length;
        int pl1 = (int)(sqrt( (plane_length*plane_length)*2 ) );

        x1 = (int)( mx+tcos[ ang ]* plane_length );
        y1 = (int)( my+tsine[ang]*plane_length );

        if ( !player.flip ) {
                x2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
        }
        else {

                x2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

}


enum build_type get_build_type( int f ) {

        enum build_type tr;

        switch ( f ) {
                case 0   :  tr = base;break;
                case 1   :  tr = office;break;
                case 2   :  tr = cow;break;
                case 3   :  tr = plant;break;
                case 4   :  tr = oil_tower;break;
                case 5   :  tr = gun;break;
                case 6   :  tr = windmill;break;
                default: tr = cow; break;
        }
        return tr;

}


int scoring( enum build_type wr ) {

        int g = wr;
        int wx = player.mission+1;

        switch ( g ) {

                case base      :  return 573*wx;
                case oil_tower :  return 475*wx;
                case cow       :  return 126*wx;
                case office    :  return 399*wx;
                case gun       :  return 346*wx;
                case windmill  :  return 425*wx;
                case plant     :  return 501*wx;

        }

        return 0;

}


bool too_close( int x, int w, int num ) {

        #define way_to_close 15

        bool cy = false;
        for ( int r = 0; r< max_buildings; r++ )
        if ( build[r].alive && r != num ) {

                if ( build[r].kind != base ) {
                        if ( x+w > build[r].x-way_to_close && x < build[r].x+build[r].width+way_to_close )
                                cy = true;
                }
                else {

                        if ( build[r].direction == true ) {
                                if ( x+w > build[r].x-way_to_close && x < build[r].x+base_length )
                                        cy = true;
                        }
                        else
                        if ( x+w > ( (build[r].x+build[r].width ) - base_length ) &&
                                x < build[r].x+build[r].width + way_to_close )
                                cy = true;

                }                                 //if kind != base

        }                                         //if r != num

        return cy;

}


void make_base( int who, bool mine ) {

        build[who].phase = random() % 50;
        build[who].alive = true;
        build[who].kind = base;
        build[who].health = 100;
        build[who].size = build[who].kind;
        build[who].width = get_width( build[who].kind );
        if ( mine )
                build[who].allegiance = 0;
        else
                build[who].allegiance = 1;

        if ( random() % 2 == 0 )
                build[who].direction = true;
        else
                build[who].direction = false;

        int min_x, max_x;
        do {
                build[who].x = random() % ( max_land - 500 ) + 250;

                if ( build[who].direction == true ) {

                        min_x = build[who].x;
                        max_x = min_x + base_length;

                }
                else {

                        max_x = build[who].x+build[who].width;
                        min_x = max_x - base_length;

                }

        } while ( too_close( min_x, (max_x-min_x), who ) || key[ K_QUIT ] );

        build[who].y = 0;

        while ( not_hit_something( build[who].x, build[who].y, build[who].width, build[who].size ) )
                build[who].y++;

        for ( int r = min_x; r <= max_x; r++ ) {

                land[ r ] = screen_y - ( build[who].y+build[who].size );

        }                                         //for

}


int get_health( enum build_type mine ) {

        int q = mine;

        switch (q) {

                case base   :  return 100;
                case cow    :  return 30;
                case plant  :  return 40;
                case office :  return 40;
                case windmill  :  return 50;
                case gun    :  return 40;
                case oil_tower :  return 50;

        }

        return 0;

}


void set_buildings() {

        for ( int r = 0; r< max_buildings; r++ ) {
                build[ r ].alive = false;
                build[ r ].health = 0;
                build[ r ].has_plane = -1;
                build[ r ].max_plane = 0;
        }

        make_base( 0, true );

        int make = mission[ player.mission ].enemy_bases;

        for ( int q = 0; q< make; q++)
                make_base( q+1, false );

        for ( int me = make+1; me < make+1+mission[ player.mission ].other_buildings ; me++ ) {

                build[me].phase = random() % 50;
                build[me].alive = true;
                build[me].rubble = false;
                build[me].kind = get_build_type( random() % 6 +1 );
                build[me].health = get_health( build[me].kind );
                build[me].size = build[me].kind;
                build[me].width = get_width( build[me].kind );

                do {
                        build[me].x = random() % ( max_land - 200 ) + 100;
                } while ( too_close( build[me].x, build[me].width, me ) || key[ K_QUIT ] );

                build[me].y = 0;

                while ( not_hit_something( build[me].x, build[me].y, build[me].width, build[me].size ) )
                        build[me].y++;

                for ( int r = build[me].x; r <= build[me].x+build[me].width; r++ ) {

                        land[ r ] = screen_y - ( build[me].y+build[me].size );

                }                                 //for

        }                                         // for

}


void rubble_building( int who ) {

        build[who].rubble = true;

}


void average_land( int * land, int use, int level ) {
        int many = 1;
        //int before = land[use];
        for ( int q = -level; q<level; q++) {
                if ( ( q + use >= 0) && ( q + use < max_land) && ( q != 0 ) ) {
                        land[use] += land[q+use];
                        many++;                   //la de da
                }
        }
        land[use] = land[use] / many;
}


void draw_land_line( BITMAP * use_land, int x, int sy, int ey ) {

        int fy2 = ey;
        int fy1 = ey - max_land_color/2;
        int color = land_color + max_land_color-1;
        while ( fy2 > sy ) {
                vline( land_work, x, fy1, fy2, color );
                fy2 = fy1-1;
                fy1 = fy2 - max_land_color/2;
                if ( fy1 < sy )
                        fy1 = sy;
                else color--;
                if ( color < land_color ) color = land_color;
        }

}


void install_land() {

        for ( int r = 0; r < max_land; r++ )
                land[r] = 1;

        //all land is now the same level

        for ( int r = 0; r < max_land / 10; r ++ ) {
                int t = random() % max_land;
                land[t] = random() % (int)(screen_x*2.35) + 50;
        }

        //set some land to be super huge

        for ( int times = 0; times < 10; times++ )
                for ( int z = 0; z < max_land; z++ )
                        average_land( land, z, 10 );

        set_buildings();

        for ( int q = 0; q < max_land; q++ )
                average_land( land, q, 2 );

        //draw it on our land bitmap
        for ( int q = 0; q < max_land; q++ )
                draw_land_line( land_work, q, max_land_height - land[q], max_land_height );
}


void install_player() {

        for ( int r = 0; r < max_buildings; r++ )
        if ( build[r].kind == base && build[r].alive == true ) {

                int min_x, mad_x;
                if ( build[r].direction == true ) {

                        min_x = build[r].x;
                        mad_x = min_x + base_length;

                }
                else {

                        mad_x = build[r].x+build[r].width;
                        min_x = mad_x - base_length;

                }

                for ( int q = min_x; q <= mad_x; q++ ) {
                        land[ q ] = screen_y - ( build[r].y+build[r].size );
                        draw_land_line(land_work,q,max_land_height - land[q], max_land_height );
                }

        }

        if ( build[0].direction == true )
                player.my_plane.virtx = build[0].x+build[0].width+15;
        else
                player.my_plane.virtx = build[0].x - 15;

        player.my_plane.virty = build[0].y+build[0].size - 5;
        player.my_plane.stall = false;

        player.my_plane.realx = (int)player.my_plane.virtx;
        player.my_plane.realy = (int)player.my_plane.virty;

        player.my_plane.landed = true;
        player.home = false;
        player.my_plane.dead_phase = 0;
        player.my_plane.showx = 320;
        player.my_plane.showy = player.my_plane.realy;
        player.shooting_ang = 0;
        player.shoot_up = false;

        if ( build[0].direction == false ) {
                player.my_plane.angle = 180;
                player.flip = true;
        }
        else {
                player.my_plane.angle = 0;
                player.flip = false;
        }

        player.my_plane.health = 50;
        player.my_plane.speed = 0;
        player.my_plane.bombs_left = max_bomb_left;
        player.my_plane.shots_left = max_shot_left;
        player.my_plane.fuel_left = max_fuel;
        player.my_plane.flip_counter = -1;
        player.my_plane.bomb_counter = -1;
        player.my_plane.shot_counter = -1;
        player.my_plane.dx = 0;
        player.my_plane.dy = 0;
        player.my_plane.allegiance = 0;

        change_dx();

}


void set_misc() {

        for ( int r = 0; r< max_enemy; r++ )
                enemy[r].alive = false;

        for ( int r = 0; r< max_expl; r++ )
                explr[r].alive = false;

        for ( int r = 0; r< max_bomb; r++ )
                bomb[r].alive = false;

        for ( int r = 0; r< max_shot; r++ )
                shot[r].life = 0;

        for ( int r = 0; r< max_buildings; r++ ) {
                build[r].alive = false;
                build[r].rubble = false;
        }

}


void set_colors() {

        RGB change;
        change.r = 0;
        change.b = 63;
        change.g = 0;

        set_color( 0, &change );

        change.r = 32;
        change.b = 32;
        change.g = 63;
        double z1 = (double)(32 - max_land_color) / 1.7;
        double z2 = (double)(63 - max_land_color) / 2.1;
        z1 = 1;
        z2 = 1;

        for ( int g = 0; g < max_land_color; g++ ) {
                change.r = 32 - (int)(g*1.3);
                change.b = 32 - (int)(g*1.3);
                change.g = 63 - (int)(g*1.9);
                set_color( land_color+g, &change );
        }

        for ( int r = expl_color; r > expl_color-10; r-- ) {
                change.r = 63;
                change.b = 63;
                change.g = 63;
                set_color( r, &change );
        }

        for ( int r = expl_color-10; r > expl_color - 32; r-- ) {

                change.g = 63 - ( expl_color-10-r );
                change.b = 0;
                change.r = 63;
                set_color( r, &change );
        }

        for ( int r = expl_color-32; r > expl_color-63; r-- ) {

                change.r = 55- ( expl_color-32-r );
                change.b = 0;
                change.g = 32 - ( expl_color-32-r );
                set_color( r, &change );
        }

}


void set_mission() {

        player.mission = 0;

        mission[0].starting_lives = 3;
        mission[0].max_plane_speed = 4.8;
        mission[0].min_plane_speed = 1.5;
        mission[0].max_player_speed = 4;
        mission[0].enemy_bases = 0;
        mission[0].other_buildings = 5;
        mission[0].min_enemy_int = 0;
        mission[0].enemy_int = 3;

        mission[1].starting_lives = 3;
        mission[1].max_plane_speed = 4.9;
        mission[1].min_plane_speed = 1.5;
        mission[1].max_player_speed = 4.5;
        mission[1].enemy_bases = 0;
        mission[1].other_buildings = 10;
        mission[1].min_enemy_int = 0;
        mission[1].enemy_int = 3;

        mission[2].starting_lives = 3;
        mission[2].max_plane_speed = 6;
        mission[2].min_plane_speed = 2;
        mission[2].max_player_speed = 5.5;
        mission[2].enemy_bases = 1;
        mission[2].other_buildings = 8;
        mission[2].min_enemy_int = 0;
        mission[2].enemy_int = 3;

        mission[3].starting_lives = 3;
        mission[3].max_plane_speed = 5;
        mission[3].min_plane_speed = 2;
        mission[3].max_player_speed = 5.5;
        mission[3].enemy_bases = 2;
        mission[3].other_buildings = 10;
        mission[3].min_enemy_int = 0;
        mission[3].enemy_int = 3;

        mission[4].starting_lives = 3;
        mission[4].max_plane_speed = 5;
        mission[4].min_plane_speed = 2;
        mission[4].max_player_speed = 6;
        mission[4].enemy_bases = 3;
        mission[4].other_buildings = 1;
        mission[4].min_enemy_int = 0;
        mission[4].enemy_int = 4;

        mission[5].starting_lives = 3;
        mission[5].max_plane_speed = 5;
        mission[5].min_plane_speed = 2;
        mission[5].max_player_speed = 6;
        mission[5].enemy_bases = 3;
        mission[5].other_buildings = 5;
        mission[5].min_enemy_int = 1;
        mission[5].enemy_int = 4;

        mission[6].starting_lives = 4;
        mission[6].max_plane_speed = 5;
        mission[6].min_plane_speed = 2;
        mission[6].max_player_speed = 6;
        mission[6].enemy_bases = 3;
        mission[6].other_buildings = 10;
        mission[6].min_enemy_int = 1;
        mission[6].enemy_int = 4;

        mission[7].starting_lives = 4;
        mission[7].max_plane_speed = 5;
        mission[7].min_plane_speed = 2;
        mission[7].max_player_speed = 6;
        mission[7].enemy_bases = 5;
        mission[7].other_buildings = 1;
        mission[7].min_enemy_int = 1;
        mission[7].enemy_int = 5;

        mission[8].starting_lives = 4;
        mission[8].max_plane_speed = 5;
        mission[8].min_plane_speed = 2;
        mission[8].max_player_speed = 6;
        mission[8].enemy_bases = 4;
        mission[8].other_buildings = 10;
        mission[8].min_enemy_int = 1;
        mission[8].enemy_int = 5;

        mission[9].starting_lives = 4;
        mission[9].max_plane_speed = 5;
        mission[9].min_plane_speed = 2;
        mission[9].max_player_speed = 6;
        mission[9].enemy_bases = 5;
        mission[9].other_buildings = 15;
        mission[9].min_enemy_int = 2;
        mission[9].enemy_int = 3;

        mission[10].starting_lives = 4;
        mission[10].max_plane_speed = 6;
        mission[10].min_plane_speed = 2.5;
        mission[10].max_player_speed = 7;
        mission[10].enemy_bases = 7;
        mission[10].other_buildings = 3;
        mission[10].min_enemy_int = 2;
        mission[10].enemy_int = 3;

        mission[11].starting_lives = 5;
        mission[11].max_plane_speed = 7;
        mission[11].min_plane_speed = 2;
        mission[11].max_player_speed = 6;
        mission[11].enemy_bases = 5;
        mission[11].other_buildings = 6;
        mission[11].min_enemy_int = 0;
        mission[11].enemy_int = 3;

        mission[12].starting_lives = 5;
        mission[12].max_plane_speed = 6;
        mission[12].min_plane_speed = 2.5;
        mission[12].max_player_speed = 5.85;
        mission[12].enemy_bases = 2;
        mission[12].other_buildings = 12;
        mission[12].min_enemy_int = 2;
        mission[12].enemy_int = 5;

        mission[13].starting_lives = 5;
        mission[13].max_plane_speed = 5.75;
        mission[13].min_plane_speed = 2.5;
        mission[13].max_player_speed = 5.75;
        mission[13].enemy_bases = 2;
        mission[13].other_buildings = 13;
        mission[13].min_enemy_int = 1;
        mission[13].enemy_int = 10;

        mission[14].starting_lives = 5;
        mission[14].max_plane_speed = 6.32;
        mission[14].min_plane_speed = 2.5;
        mission[14].max_player_speed = 6.32;
        mission[14].enemy_bases = 3;
        mission[14].other_buildings = 10;
        mission[14].min_enemy_int = 5;
        mission[14].enemy_int = 5;

}


void first_init() {
        #define dot printf(".");
        printf("Loading game.");
        allegro_init(); dot
        install_sound( DIGI_AUTODETECT, MIDI_NONE, "" ); dot
        install_timer(); dot
        install_keyboard(); dot
        data_snd = load_datafile( "sound.dat" );
        if ( !data_snd ) {
                printf("Error with datafile!\n");
                exit( 1 );
        }
        snd[ snd_death ]       = (SAMPLE *)data_snd[ DYING ].dat;
        snd[ snd_hurt_1 ]      = (SAMPLE *)data_snd[ OW1 ].dat;
        snd[ snd_hurt_2 ]      = (SAMPLE *)data_snd[ OW2 ].dat;
        snd[ snd_hurt_3 ]      = (SAMPLE *)data_snd[ OW3 ].dat;
        snd[ snd_destroy ]     = (SAMPLE *)data_snd[ SMALLEX ].dat;
        snd[ snd_shoot ]       = (SAMPLE *)data_snd[ LF3 ].dat;
        snd[ snd_bomb_away ]   = (SAMPLE *)data_snd[ BOMBAWAY ].dat;
        snd[ snd_bomb_hit ]    = (SAMPLE *)data_snd[ BOMB ].dat;
        snd[ snd_plane_crash ] = (SAMPLE *)data_snd[ LARGEEX ].dat;
        //snd[ snd_theme_song ]  = load_sample( "theme.wav" );dot

        snd[ snd_death ]->priority       = 252;
        snd[ snd_hurt_1 ]->priority      = 249;
        snd[ snd_hurt_2 ]->priority      = 249;
        snd[ snd_hurt_3 ]->priority      = 249;
        snd[ snd_destroy ]->priority     = 251;
        snd[ snd_shoot ]->priority       = 247;
        snd[ snd_bomb_away ]->priority   = 248;
        snd[ snd_bomb_hit ]->priority    = 248;
        snd[ snd_plane_crash ]->priority = 253;
        //snd[ snd_theme_song ]->priority  = 255;
        printf("\n");

        set_trig();
        srandom( time( NULL ) );
        set_color_depth( 8 );

	#ifdef PANDORA
        set_gfx_mode( GFX_AUTODETECT_FULLSCREEN, screen_x, screen_y, screen_x, screen_y );
	#else
        set_gfx_mode( GFX_AUTODETECT_WINDOWED, screen_x, screen_y, screen_x, screen_y );
	#endif

        // text_mode(-1);
        get_palette( original );

        work = create_bitmap( screen_x, screen_y );
        clear( work );
        land_work = create_bitmap( max_land, max_land_height );

        minor_init();

}


void minor_init() {
        clear( land_work );
        player.mission = 0;
        player.lives = mission[ player.mission ].starting_lives;
        player.score = 0;                         //this is in the right spot
        player.real_score = 0;
        player.next_level = 1000;
        set_mission();
        set_misc();
        install_land();
        re_init_player();
        set_colors();
}


void draw_player() {

        int ph = (int)( (double)player.my_plane.health / ( (double)max_health / 100.0 ) );
        int pf = (int)( (double)player.my_plane.fuel_left / ( (double)max_fuel / 100.0 ) );
        int pb = (int)( (double)player.my_plane.bombs_left/( (double)max_bomb_left / 100.0 ) );
        int ps = (int)( (double)player.my_plane.shots_left/( (double)max_shot_left / 100.0 ) );

        rectfill( work, 10, 400, 10 + ph, 410, 35 );
        rect( work, 10, 400, 10 + ph, 410, 24 );
        rectfill( work, 10, 415, 10 + pf, 425, 39 );
        rect( work, 10, 415, 10 + pf, 425, 24 );
        rectfill( work, 10, 430, 10 + pb, 440, 45 );
        rect( work, 10, 430, 10 + pb, 440, 24 );
        rectfill( work, 10, 445, 10 + ps, 455, 49 );
        rect( work, 10, 445, 10 + ps, 455, 24 );

	char * str = int2str( player.score );
        textprintf_ex( work, font, 320, 465, 23, -1, "SCORE: [%s] LIVES:[%d]", str, player.lives );
	delete[] str;

        int plane_length = gen_plane_length;

        int ox, oy;

        ox = player.my_plane.showx;
        oy = player.my_plane.showy;

        int x1, y1, x2, y2, x3, y3;
        int ang = player.my_plane.angle;

        int pl1 = (int)( sqrt( (plane_length*plane_length)*2 ) );

        x1 = (int)( ox+tcos[ ang ]* plane_length );
        y1 = (int)( oy+tsine[ang]*plane_length );

        if ( !player.flip ) {

                x2 = (int)( ox + tcos[ (ang+angular_1) % 360 ] * pl1 );
                y2 = (int)( oy + tsine[ (ang+angular_1) % 360 ] * pl1 );

                x3 = (int)( ox + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( oy + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }
        else {

                x2 = (int)( ox + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                y2 = (int)( oy + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                x3 = (int)( ox + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( oy + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

        triangle( work, x1, y1, x2, y2, x3, y3, 39 );

}


void draw_cow( int x1, int y1, int x2, int y2 ) {

        int lengthx = x2 - x1;
        int lengthy = y2 - y1;

        ellipsefill( work, x1+lengthx / 2, y1+lengthy /2, lengthx/2, lengthy / 3, 35 );
        rectfill( work, x1, y1+lengthy/4, x1+lengthx/3, y2, 35 );
        rectfill( work, x2-lengthx/3, y1+lengthy/4, x2, y2, 35 );
        circlefill( work, x1, y1, 3, 35 );

}


void draw_plant( int x1, int y1, int x2, int y2, int phase ) {

        int xl = x2 - x1;
        int yl = y2 - y1;

        rectfill( work, x1, y1, x2, y2, 43 );
        int xr = 0, yr = 0;
        phase = ( phase / 2 ) % 21;

        switch ( phase ) {

                case 0   :
                case 20  :  { xr = 10; yr = 1; break; }
                case 1   :
                case 19  :  { xr = 8; yr = 2; break; }
                case 2   :
                case 18  :  { xr = 6; yr = 4; break; }
                case 3   :
                case 17  :  { xr = 4; yr = 6; break; }
                case 4   :
                case 16  :  { xr = 2; yr = 8; break; }
                case 5   :
                case 15  :  { xr = 1; yr = 10; break; }
                case 6   :
                case 14  :  { xr = 2; yr = 8; break; }
                case 7   :
                case 13  :  { xr = 4; yr = 6; break; }
                case 8   :
                case 12  :  { xr = 6; yr = 4; break; }
                case 9   :
                case 11  :  { xr = 8; yr = 2; break; }
                case 10  :  { xr = 10; yr = 1; break; }

        }

        ellipsefill( work, x1+ xl / 2, y1+ yl / 2+3, xr, yr, 39 );
        rectfill( work, x1, y1-5, x1+2, y1, 32+72 );
        rectfill( work, x2, y1-5, x2-2, y1, 32+72 );

        int ox, oy;
        int nx, ny;
        ox = x1;
        oy = y1-6;
        while ( ox < x2-4 ) {
                nx = ox+2;
                if ( random() % 2 == 0 )
                        ny = oy + random() % 2;
                else
                        ny = oy - random() % 2;

                if ( ny > y1-2 )
                        ny = y1-2;
                if ( ny < y1-10 )
                        ny = y1-10;

                line( work, ox, oy, nx, ny, 31 );
                ox = nx;
                oy = ny;

        }
        line( work, ox, oy, x2, y1-5, 31 );
        textprintf_ex( work, font, x1, y1+2, 39, -1, "POWER" );

}


void draw_mill( int x1, int y1, int x2, int y2, int phase ) {

        triangle( work, x1, y2, x2, y2, (x1+x2)/2, (y1+y2)/2, 45+72 );
        int xl = x2 - x1;
        int yl = y2 - y1;

                                                  /*+ yl / 3*/
        rectfill( work, x1+xl / 3, y1 , x2- xl / 3, y2- yl / 2, 46+72 );

        struct xy_rec { int x; int y; };
        xy_rec fan[ 4 ];

        int mx = ( x1+x2 ) / 2;
        int my = ( y1+y2 ) / 2;

        int max = 5;

        phase = phase / ( 50 / max );
        if ( phase > max-1 )
                phase = max-1;

        switch ( phase ) {
                case 0   : {
                        fan[0].x = mx-6;
                        fan[0].y = my;
                        fan[1].x = mx;
                        fan[1].y = my-6;
                        fan[2].x = mx+6;
                        fan[2].y = my;
                        fan[3].x = mx;
                        fan[3].y = my+6;
                        break;
                }
                case 1   : {
                        fan[0].x = mx-5;
                        fan[0].y = my-2;

                        fan[1].x = mx+2;
                        fan[1].y = my-5;

                        fan[2].x = mx+5;
                        fan[2].y = my+2;

                        fan[3].x = mx-2;
                        fan[3].y = my+5;

                        break;
                }
                case 2   : {
                        fan[0].x = mx-4;
                        fan[0].y = my-3;

                        fan[1].x = mx+3;
                        fan[1].y = my-4;

                        fan[2].x = mx+4;
                        fan[2].y = my+3;

                        fan[3].x = mx-3;
                        fan[3].y = my+4;

                        break;
                }
                case 3   : {
                        fan[0].x = mx-3;
                        fan[0].y = my-4;

                        fan[1].x = mx+4;
                        fan[1].y = my-3;

                        fan[2].x = mx+3;
                        fan[2].y = my+4;

                        fan[3].x = mx-4;
                        fan[3].y = my+3;

                        break;
                }
                case 4   : {
                        fan[0].x = mx-2;
                        fan[0].y = my-5;

                        fan[1].x = mx+5;
                        fan[1].y = my-2;

                        fan[2].x = mx+2;
                        fan[2].y = my+5;

                        fan[3].x = mx-5;
                        fan[3].y = my+2;

                        break;
                }
        }
        for ( int r = 0; r< 4; r++ )
                circlefill( work, fan[r].x, fan[r].y, 3, 43 );

}


void draw_base( int x1, int y1, int x2, int y2, int phase, int all ) {

        if ( all == 0 )
                rectfill( work, x1, y1+5, x2, y2, 65 );
        else
                rectfill( work, x1, y1+5, x2, y2, 53 );

        rectfill( work, x1+2, y2-10, x1+6, y2, 46+72 );
        rectfill( work, x2-2, y2-10, x2-7, y2, 46+72 );

        int mx = ( x1+x2 ) / 2;
        int my = y1;

        phase = ( phase * 4 / 5 ) % 18;

        switch ( phase ) {

                case 0   : {

                        ellipsefill( work, mx+4, my, 2, 5, 31 );
                        ellipsefill( work, mx+5, my, 1, 4, 30 );
                        line( work, mx+4, my, mx+8, my, 39 );

                        break;

                }

                case 1   : {

                        ellipsefill( work, mx+3, my, 3, 5, 31 );
                        ellipsefill( work, mx+4, my, 2, 4, 30 );
                        ellipsefill( work, mx+5, my, 1, 3, 29 );
                        line( work, mx+3, my, mx+7, my, 39 );

                        break;

                }

                case 2   : {

                        ellipsefill( work, mx+2, my, 4, 5, 31 );
                        ellipsefill( work, mx+3, my, 3, 4, 30 );
                        ellipsefill( work, mx+3, my, 2, 3, 29 );
                        ellipsefill( work, mx+4, my, 1, 2, 28 );
                        line( work, mx+2, my, mx+5, my+1, 39 );

                        break;

                }

                case 3   : {

                        ellipsefill( work, mx+1, my, 5, 5, 31 );
                        ellipsefill( work, mx+2, my, 4, 4, 30 );
                        ellipsefill( work, mx+2, my, 3, 3, 29 );
                        ellipsefill( work, mx+3, my, 2, 2, 28 );
                        ellipsefill( work, mx+3, my, 1, 1, 27 );
                        line( work, mx+1, my, mx+4, my+1, 39 );

                        break;

                }

                case 4   : {

                        ellipsefill( work, mx, my, 5, 5, 31 );
                        ellipsefill( work, mx, my, 4, 4, 30 );
                        ellipsefill( work, mx, my, 3, 3, 29 );
                        ellipsefill( work, mx, my, 2, 2, 28 );
                        ellipsefill( work, mx, my, 1, 1, 27 );
                        line( work, mx, my, mx+3, my, 39 );

                        break;

                }

                case 5   : {

                        line( work, mx-1, my, mx-4, my+1, 39 );
                        ellipsefill( work, mx-1, my, 5, 5, 31 );
                        ellipsefill( work, mx-2, my, 4, 4, 30 );
                        ellipsefill( work, mx-2, my, 3, 3, 29 );
                        ellipsefill( work, mx-3, my, 2, 2, 28 );
                        ellipsefill( work, mx-3, my, 1, 1, 27 );

                        break;

                }

                case 6   : {

                        line( work, mx-2, my, mx-5, my+1, 39 );
                        ellipsefill( work, mx-2, my, 4, 5, 31 );
                        ellipsefill( work, mx-3, my, 3, 4, 30 );
                        ellipsefill( work, mx-3, my, 2, 3, 29 );
                        ellipsefill( work, mx-4, my, 1, 2, 28 );

                        break;

                }

                case 7   : {

                        line( work, mx-3, my, mx-7, my, 39 );
                        ellipsefill( work, mx-3, my, 3, 5, 31 );
                        ellipsefill( work, mx-4, my, 2, 4, 30 );
                        ellipsefill( work, mx-5, my, 1, 3, 29 );

                        break;

                }

                case 8   : {

                        line( work, mx-4, my, mx-8, my, 39 );
                        ellipsefill( work, mx-4, my, 2, 5, 31 );
                        ellipsefill( work, mx-5, my, 1, 4, 30 );

                        break;

                }

                case 9   : {
                        ellipsefill( work, mx-4, my, 1, 5, 31 );
                        break;
                }

                case 10   : {
                        ellipsefill( work, mx-3, my, 2, 5, 31 );
                        break;
                }

                case 11   : {
                        ellipsefill( work, mx-2, my, 3, 5, 31 );
                        break;
                }

                case 12   : {
                        ellipsefill( work, mx-1, my, 4, 5, 31 );
                        break;
                }

                case 13   : {
                        ellipsefill( work, mx, my, 5, 5, 31 );
                        break;
                }

                case 14   : {
                        ellipsefill( work, mx+1, my, 4, 5, 31 );
                        break;
                }

                case 15   : {
                        ellipsefill( work, mx+2, my, 3, 5, 31 );
                        break;
                }

                case 16   : {
                        ellipsefill( work, mx+3, my, 2, 5, 31 );
                        break;
                }

                case 17   : {
                        ellipsefill( work, mx+4, my, 1, 5, 31 );
                        break;
                }

        }

}


void draw_office( int x1, int y1, int x2, int y2, int phase ) {

        rectfill( work, x1, y1, x2, y2, 35+72 );
        bool window[ 9 ];

        phase = ( phase / 6 ) % 6;
        switch ( phase ) {

                case 0   : {
                        window[0] = false;
                        window[1] = true;
                        window[2] = true;
                        window[3] = false;
                        window[4] = true;
                        window[5] = false;
                        window[6] = true;
                        window[7] = true;
                        window[8] = false;
                        break;
                }                                 // 0

                case 1   : {
                        window[0] = false;
                        window[1] = false;
                        window[2] = true;
                        window[3] = false;
                        window[4] = true;
                        window[5] = false;
                        window[6] = false;
                        window[7] = true;
                        window[8] = false;
                        break;
                }                                 // 1

                case 2   : {
                        window[0] = true;
                        window[1] = false;
                        window[2] = true;
                        window[3] = false;
                        window[4] = false;
                        window[5] = true;
                        window[6] = false;
                        window[7] = true;
                        window[8] = false;
                        break;
                }                                 // 2

                case 3   : {
                        window[0] = true;
                        window[1] = false;
                        window[2] = true;
                        window[3] = false;
                        window[4] = true;
                        window[5] = true;
                        window[6] = false;
                        window[7] = false;
                        window[8] = true;
                        break;
                }                                 // 3

                case 4   : {
                        window[0] = false;
                        window[1] = false;
                        window[2] = false;
                        window[3] = true;
                        window[4] = true;
                        window[5] = true;
                        window[6] = false;
                        window[7] = true;
                        window[8] = false;
                        break;
                }                                 // 4

                case 5   : {
                        window[0] = false;
                        window[1] = true;
                        window[2] = false;
                        window[3] = true;
                        window[4] = false;
                        window[5] = true;
                        window[6] = true;
                        window[7] = false;
                        window[8] = false;
                        break;
                }                                 // 5

        }                                         //switch

        for ( int r = 0; r< 9; r++ ) {
                int ax1 = x1 + ( r % 3 ) * 12+ 2;
                int ay1 = y1 + ( r / 3 ) * 12 + 2;
                if ( window[r] )
                        rectfill( work, ax1, ay1, ax1+5, ay1+5, 41 );
                else
                        rectfill( work, ax1, ay1, ax1+5, ay1+5, 22 );
        }

}


void draw_rubble( int x1, int y1, int x2 ) {

        int many = random() % 5 + 3;
        for ( int r = 0; r< many; r++ ) {

                int x, y, mx;

                x = random() % ( x2-x1 ) + x1;
                y = y1 - ( random() % 10 + 4 );
                mx = random() % ( x2 - x1 ) + x1;
                if ( x > mx ) {
                        int w = mx;
                        mx = x;
                        x = w;
                }

                triangle( work, x, y1, (x+mx)/2, y, mx, y1, 39 );

        }

        rectfill( work, x1, y1-3, x2, y1, 27 );
        rectfill( work, (x1+x2-2)/2, y1, (x1+x2+2)/2, y1-4, 27 );

}


void draw_gun( int x1, int y1, int x2, int y2, int phase ) {

        int col = 45+72;
        rectfill( work, x1 + 5, y1, x2 - 10, y1 + 6, col );
        rectfill( work, x2-9, y1 + 2, x2, y1 + 4, col );
        rectfill( work, x1, y1 + 7, x2, y2, col );

        phase = (int)( (double)phase / ( 40.0 / 360.0 ) );

        int ang = phase;
        int mx = x1 + 12;
        int my = y2 - 7;

        int lex = 5;
        int ax1 = (int)( (double)mx + tcos[ ang ] * lex );
        int ay1 = (int)( (double)my + tsine[ ang ] * lex );

        int bx1 = (int)( (double)mx + tcos[ (ang+120)%360 ] * lex );
        int by1 = (int)( (double)my + tsine[ (ang+120)%360 ] * lex );

        int cx1 = (int)( (double)mx + tcos[ (ang+240)%360 ] * lex );
        int cy1 = (int)( (double)my + tsine[ (ang+240)%360 ] * lex );

        triangle( work, ax1, ay1, bx1, by1, cx1, cy1, 31 );

}


void draw_tower( int x1, int y1, int x2, int y2, int phase ) {

        rectfill( work, x1+3, y1, x2-3, y2, 65 );
        rectfill( work, x1+10, y1+5, x2-10, y2-5, 0 );

        phase %= 32;
        int ang = 0;
        switch( phase ) {
                case 0   : {ang = 360;break;}
                case 1   : {ang = 355;break;}
                case 2   : {ang = 350;break;}
                case 3   : {ang = 340;break;}
                case 4   : {ang = 330;break;}
                case 5   : {ang = 315;break;}
                case 6   : {ang = 300;break;}
                case 7   : {ang = 280;break;}
                case 8   : {ang = 260;break;}
                case 9   : {ang = 245;break;}
                case 10   : {ang = 230;break;}
                case 11   : {ang = 220;break;}
                case 12   : {ang = 210;break;}
                case 13   : {ang = 200;break;}
                case 14   : {ang = 190;break;}
                case 15   : {ang = 185;break;}
                case 16   : {ang = 180;break;}
                case 17   : {ang = 185;break;}
                case 18   : {ang = 190;break;}
                case 19   : {ang = 200;break;}
                case 20   : {ang = 210;break;}
                case 21   : {ang = 220;break;}
                case 22   : {ang = 230;break;}
                case 23   : {ang = 245;break;}
                case 24   : {ang = 260;break;}
                case 25   : {ang = 280;break;}
                case 26   : {ang = 300;break;}
                case 27   : {ang = 315;break;}
                case 28   : {ang = 330;break;}
                case 29   : {ang = 340;break;}
                case 30   : {ang = 350;break;}
                case 31   : {ang = 355;break;}
        }

        struct xxyy {

                int x, y;

        };
        #define max_ruff 10
        #define lexx 20
        struct xxyy ruff[ max_ruff ];

        int dang = (int)( (double)ang / ( 360.0 / 256 ) );
        for ( int r = 0; r < max_ruff; r++ ) {

                ruff[r].x = (int)( (double)(x1+r+8) + tcos[ ang ] * lexx );
                ruff[r].y = (int)( (double)y1 + tsine[ ang ] * lexx );
                line( work, x1+r+8, y1, ruff[r].x, ruff[r].y, 65+72 );
                ruff[r].y = (int)( (double)(y1+1) + tsine[ ang ] * lexx );
                line( work, x1+r+8, y1, ruff[r].x, ruff[r].y, 65+72 );
                ruff[r].y = (int)( (double)(y1+2) + tsine[ ang ] * lexx );
                line( work, x1+r+8, y1, ruff[r].x, ruff[r].y, 65+72 );
                ruff[r].y = (int)( (double)(y1+3) + tsine[ ang ] * lexx );
                line( work, x1+r+8, y1, ruff[r].x, ruff[r].y, 65+72 );
                ruff[r].y = (int)( (double)(y1+4) + tsine[ ang ] * lexx );
                line( work, x1+r+8, y1, ruff[r].x, ruff[r].y, 65+72 );
                ruff[r].y = (int)( (double)(y1+5) + tsine[ ang ] * lexx );
                line( work, x1+r+8, y1, ruff[r].x, ruff[r].y, 65+72 );

        }
        int da, db;
        da = dang - 30;
        db = dang + 30;
        if ( da < 0 )
                da += 255;
        if ( db > 255 )
                db -= 255;
        arc( work, ruff[max_ruff/2].x, ruff[max_ruff/2].y, itofix(da), itofix(db), 10, 31 );
        arc( work, ruff[max_ruff/2].x, ruff[max_ruff/2].y, itofix(da), itofix(db), 9, 31 );
        for ( int r = 0; r< 10; r++ )
                putpixel( work, x1+random() % 20 + 5, y1-random()%10 - 1, 16 );

}


void draw_buildings() {

        for ( int r = 0; r< max_buildings; r++ )
        if ( build[r].alive ) {
                build[r].phase++;

                if ( build[r].kind == oil_tower )
                        if (build[r].phase > 31 )
                                build[r].phase = 0;

                if ( build[r].phase > 40 )
                        build[r].phase = 0;

                int x1 = build[r].x;
                int y1 = build[r].y;
                int ph = build[r].phase;

                if ( x1 >= camera && ( x1 <= camera + screen_x ) ) {

                        x1 = x1 - camera;

                        if ( player.my_plane.realy - minimum < 0 )
                                y1 = y1 - ( player.my_plane.realy - minimum );

                        int x2 = x1 + build[r].width;
                        int y2 = y1 + build[r].size;

                        if ( build[r].rubble )
                                draw_rubble( x1, y2, x2 );
                        else {

                                int hg = build[r].kind;
                                switch ( hg ) {
                                        case cow       :  draw_cow( x1, y1, x2, y2 ); break;
                                        case office    :  draw_office( x1, y1, x2, y2, ph ); break;
                                        case plant     :  draw_plant( x1, y1, x2, y2, ph ); break;
                                        case gun       :  draw_gun( x1, y1, x2, y2, ph ); break;
                                        case windmill  :  draw_mill( x1, y1, x2, y2, ph ); break;
                                        case oil_tower :  draw_tower( x1, y1, x2, y2, ph ); break;
                                        case base      :  draw_base( x1, y1, x2, y2, ph, build[r].allegiance ); break;
                                }                 // switch

                        }                         // if rubble

                }                                 // if camera

        }                                         //if alive

}                                                 //draw


void hurt_player( int much ) {

        int pl = random() % 3;
        switch( pl ) {
                case 0  :       play_sample( snd[snd_hurt_1],255,128,1000,false ); break;
                case 1  :       play_sample( snd[snd_hurt_2],255,128,1000,false ); break;
                case 2  :       play_sample( snd[snd_hurt_3],255,128,1000,false ); break;
        }
        player.my_plane.health -= much;
        if ( player.my_plane.health <= 0 )
                kill_player();

}


void hurt_enemy( int who, int much ) {

        enemy[who].my_plane.health -= much;
        if ( enemy[who].my_plane.health <= 0 ) {

                enemy[who].alive = false;
                make_plane_explosion( enemy[who].my_plane.realx, enemy[who].my_plane.realy );

        }

}


void draw_expl() {

        for ( int r = 0; r< max_expl; r++ )
        if ( explr[r].alive ) {

                explr[r].vx += explr[r].wx;
                explr[r].vy += explr[r].wy;
                explr[r].x = (int)explr[r].vx;
                explr[r].y = (int)explr[r].vy;

                int rx = explr[r].x;
                int color = explr[r].radius;
                color = (int)( ( (double)(explr[r].max_rad - color ) / (double)explr[r].max_rad ) * 100.0 );
                color = color / ( 100 / 63 );
                if ( color > 63 )
                        color = 63;
                if ( color < 0 )
                        color = 0;
                //this used to be the color:( explr[r].max_rad - explr[r].radius )*2 );
                if ( rx >= camera && rx <= camera + screen_x ) {
                        int ry = explr[r].y;
                        rx -= camera;

                        if ( player.my_plane.realy - minimum < 0 )
                                ry = ry - ( player.my_plane.realy - minimum );

                        circlefill( work, rx, ry, explr[r].radius, expl_color - color );

                }

                if ( dist( explr[r].x, explr[r].y, player.my_plane.realx, player.my_plane.realy ) < explr[r].radius && player.my_plane.dead_phase <= 0 )
                        hurt_player( explr[r].radius / 3 );

                for ( int q = 0; q< max_enemy; q++ )
                        if ( enemy[q].alive )
                                if ( dist( explr[r].x, explr[r].y, enemy[q].my_plane.realx, enemy[q].my_plane.realy ) < explr[r].radius )
                                        hurt_enemy( q, explr[r].radius / 3 );

                explr[r].radius--;
                if ( explr[r].radius <= 0 )
                        explr[r].alive = false;

        }

}


void draw_bomb() {

        for ( int r = 0; r < max_bomb; r++ )
        if ( bomb[r].alive ) {

                int x1, y1, x2, y2, x3, y3;
                bomb[r].x = (int)bomb[r].vx;
                bomb[r].y = (int)bomb[r].vy;
                x1 = bomb[r].x;
                y1 = bomb[r].y;

                if ( x1 >= camera && x1 < camera + screen_x ) {

                        x1 = x1-camera;

                        if ( player.my_plane.realy - minimum < 0 )
                                y1 = y1 - ( player.my_plane.realy - minimum );

                        int ang;
                        if ( bomb[r].wx != 0 && bomb[r].wy != 0 )
                                ang = (int)( 0.5 + atan2( -bomb[r].wy, bomb[r].wx ) * 180.0 / M_PI );
                        else
                        if ( bomb[r].wx != 0 && bomb[r].wy == 0 ) {

                                if ( bomb[r].wx > 0 )
                                        ang = 0;
                                else
                                        ang = 180;

                        }
                        else {

                                if ( bomb[r].wy > 0 )
                                        ang = 270;
                                else
                                        ang = 90;

                        }

                        while ( ang < 0 )
                                ang += 360;

                        ang %= 360;

                        int size = 6;
                        x2 = (int)( x1 + tcos[ (ang+160) % 360 ] * size );
                        y2 = (int)( y1 + tsine[ (ang+160) % 360 ] * size );

                        x3 = (int)( x1 + tcos[ (ang+200) % 360 ] * size );
                        y3 = (int)( y1 + tsine[ (ang+200) % 360 ] * size );

                        triangle( work, x1, y1, x2, y2, x3, y3, 39 );

                }                                 //if > camera && < camera+screen_x

        }

}


void draw_shot() {

        for ( int r = 0; r < max_shot; r++ )
        if ( shot[r].life > 0 ) {

                shot[r].x = (int)shot[r].vx;
                shot[r].y = (int)shot[r].vy;
                int x1 = shot[r].x;
                int y1 = shot[r].y;

                if ( x1 >= camera && x1 <= camera+screen_x ) {

                        x1 = x1 - camera;
                        if ( player.my_plane.realy - minimum < 0 )
                                y1 = y1 - ( player.my_plane.realy - minimum );
                        circlefill( work, x1, y1, 1, 31 );

                }

        }

}


void draw_map() {

        //int dif = max_land / 200;
        int ml_x = 10;
        int mg_x = 215;
        int mg_y = 60;

        //rectfill( work, ml_x, mg_y-60, mg_x, mg_y, 0 );

        if ( build[0].alive && build[0].rubble == false) {

                int go_for_it = max_land / 200;

                for ( int r = 0; r< max_land; r+=go_for_it ) {

                        //draw_land_line( work, ml_x + r / go_for_it, mg_y-land[r] / 10, mg_y );
                        vline( work, ml_x + r / go_for_it, mg_y-land[r] / 10, mg_y, dark_land_color );

                }
                if ( mg_y - ( 480-player.my_plane.realy)/10 > 10 )
                        circlefill( work, ml_x + player.my_plane.realx / go_for_it,
                                mg_y - (480 - player.my_plane.realy ) / 10, 1, 39 );

                int color = grand_clock / 4;
                if ( color % 2 == 0 ) {
                        map_color++;
                        if ( map_color >= 72 )
                                map_color = 36;
                }
                color = map_color;

                for ( int r = 0; r<max_buildings; r++ )
                        if ( build[r].alive && build[r].rubble == false )
                                circlefill( work, ml_x + build[r].x / go_for_it,
                                        mg_y - (480 - (build[r].y+build[r].size) ) / 10, 1, color );

                for ( int r = 0; r< max_enemy; r++ )
                        if ( enemy[r].alive )
                                circlefill( work, ml_x+enemy[r].my_plane.realx / go_for_it,
                                        mg_y - ( 480 - enemy[r].my_plane.realy ) / 10, 1, 45 );

        }
        else
                for ( int pl = 0; pl < 120; pl++ )
                        //putpixel( work, ml_x + random() % (mg_x-ml_x),
                        //                random() % (mg_y-48) , 31 );
                        circlefill( work, ml_x + random() % (mg_x-ml_x),
                                random() % (mg_y-12) + 12, 1, 31 );

        rect( work, ml_x, 10, mg_x, mg_y, 26 );

}


void draw_enemy() {

        for ( int r = 0; r< max_enemy; r++ )
        if ( enemy[r].alive ) {

                int x1 = enemy[r].my_plane.realx;
                if ( x1 > camera && x1 <= camera+screen_x ) {

                        x1 = x1 - camera;
                        int y1 = enemy[r].my_plane.realy;
                        if ( player.my_plane.realy - minimum < 0 )
                                y1 = y1 - ( player.my_plane.realy - minimum );

                        int plane_length = gen_plane_length;

                        int ox, oy;

                        ox = x1;
                        oy = y1;

                        int x2, y2, x3, y3;
                        int ang = enemy[r].my_plane.angle;

                        int pl1 = (int)( sqrt( (plane_length*plane_length)*2 ) );

                        x1 = (int)( ox+tcos[ ang ]* plane_length );
                        y1 = (int)( oy+tsine[ang]*plane_length );

                        if ( !enemy[r].flip ) {

                                x2 = (int)( ox + tcos[ (ang+angular_1) % 360 ] * pl1 );
                                y2 = (int)( oy + tsine[ (ang+angular_1) % 360 ] * pl1 );

                                x3 = (int)( ox + tcos[ (ang+angular_2) % 360 ] * plane_length );
                                y3 = (int)( oy + tsine[ (ang+angular_2) % 360 ] * plane_length );

                        }
                        else {

                                x2 = (int)( ox + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                                y2 = (int)( oy + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                                x3 = (int)( ox + tcos[ (ang+angular_2) % 360 ] * plane_length );
                                y3 = (int)( oy + tsine[ (ang+angular_2) % 360 ] * plane_length );

                        }

                        triangle( work, x1, y1, x2, y2, x3, y3, 43+ r * 3 / 2 );

                }

        }

}


void draw_environment() {
        int ty = 0;

        if ( player.my_plane.realy - minimum < 0 )
                ty = ty + ( player.my_plane.realy - minimum );

        blit( land_work, work, camera, ty, 0, screen_y-max_land_height, screen_x, max_land_height );

        draw_buildings();
        if ( player.my_plane.dead_phase <= 0 )
                draw_player();
        draw_enemy();
        draw_expl();
        draw_bomb();
        draw_shot();
        draw_map();
        //textprintf( work, font, 1, 1, 31, "%d", player.my_plane.dead_phase );
        //textprintf( work, font, 1, 1, 31, "%.2f", player.my_plane.speed );
        //textprintf( work, font, 1, 20, 31, "X:%d Y:%d", player.my_plane.realx, player.my_plane.realy );

        //blur( work );
        blit( work, screen, 0, 0, 0, 0, screen_x, screen_y);

	/*
        RGB change;

        change.r = 0;
        if ( ultra_color == 0 ) {
                change.g = 0;
                change.b = 63;
        }
        else {
                change.b = 0;
                change.g = 63;
        }

        int q = (player.my_plane.realy+1500);

        q = q / 30;
        if ( q < 0 )
                q = 0;
        if ( q > 63 )
                q = 63;

        if ( ultra_color == 0 )
                change.b = q;
        else
                change.g = q;

        set_color( 0, &change );
	*/

	RGB change;
	get_color( 0, &change );
	int q = (player.my_plane.realy+1500);
        q = q / 30;
        if ( q < 0 )
                q = 0;
        if ( q > 63 )
                q = 63;
	if ( change.b != q ){
		change.b = q;
		set_color(0,&change);
	}

        clear( work );

}


void re_init_player() {

        install_player();
        for ( int r = 0; r< max_buildings; r++ )
                build[r].rubble = false;

}


void eeek( int mx, int my, double dx ) {
        for ( int ang = 0; ang <= 180; ang+=30 )
                make_explosion( (int)(mx+tcos[ang]),(int)(my+tsine[ang]),tcos[ang]*dx,tsine[ang]*dx,35 );

}


void explode_building( int mx, int my, enum build_type guess ) {

        int what = guess;

        switch (what) {

                case cow :  { eeek( mx, my, 0 ); break; }
                case office :  { eeek( mx, my, 2.3 ); break; }
                case gun :  { eeek( mx, my, 2.8 ); break; }
                case plant  :  { eeek( mx, my, 3.7 ); break; }
                case oil_tower :  { eeek( mx, my, 1.5 ); eeek( mx, my, 4.5 ); break; }
                case base   :  { eeek( mx, my, 3.0 ); break; }
                case windmill  :  { eeek( mx, my, 2.1 ); break; }

        }

}


bool hit_building( int x, int y) {

        if ( x < 0 || x >= max_land )
                return false;

        bool cy = false;
        for ( int r = 0; r < max_buildings; r++ )
        if ( build[r].alive ) {
                int x1, y1, x2, y2;

                x1 = build[r].x;
                x2 = build[r].x+build[r].width;
                y1 = build[r].y;
                y2 = build[r].y+build[r].size;

                if ( build[r].rubble == true )
                        y1 = y2-5;

                if ( x >= x1 && x <= x2 &&
                y >= y1 && y <= y2 ) {
                        cy = true;
                        if ( build[r].rubble == false ) {
                                play_sample( snd[ snd_destroy ], 255, 128, 1000, false );
                                explode_building( (x1+x2)/2, (y1+y2)/2, build[r].kind );
                                player.score += scoring( build[r].kind );
                                player.real_score += scoring( build[r].kind );
                                if ( r == 0 )
                                        player.score -= 5000;
                        }

                        rubble_building( r );
                }
        }

        return cy;

}


int touch_building( int x, int y ) {

        if ( x < 0 || x >= max_land )
                return -1;

        for ( int r = 0; r < max_buildings; r++ )
        	if ( build[r].alive ) {
                	int x1, y1, x2, y2;

	                x1 = build[r].x;
        	        x2 = build[r].x+build[r].width;
                	y1 = build[r].y;
	                y2 = build[r].y+build[r].size;
	
        	        if ( build[r].rubble == true )
                	        y1 = y2-5;
	
        	        if ( x >= x1 && x <= x2 &&
                	        y >= y1 && y <= y2 )
                        	return r;                 /*then*/

        	}

        return -1;

}


void chop_land( int wx, int wy, int power ) {

        bool temp[ max_land ];
        for ( int r = 0; r< max_land; r++ )
                temp[r] = false;

        int ang, rx, ry;

        for ( ang =1; ang < 180; ang++ ) {

                rx = (int)( wx + 0.5 + tcos[ ang ] * power );

                if ( rx >= 0 && rx < max_land )
                if ( temp[rx] == false ) {

                        temp[rx] = true;
                        ry = (int)( wy + 0.5 + tsine[ ang ] * power );
                        int cy;
                        cy = (int)( wy + 0.5 + tsine[ 360-ang] * power );
                        if ( cy < 1 )
                                cy = 1;

                        vline( land_work, rx, 0, max_land_height, 0 );
                        if ( ry >= screen_y - land[ rx ] )
                                land[ rx ] -= cy - ry;
                        else
                        if ( cy >= screen_y - land[ rx ] )
                                land[ rx ] -= cy - ( screen_y - land[ rx ] );

                        if ( land[ rx ] < 1 )
                                land[ rx ] = 1;
                        draw_land_line( land_work, rx, max_land_height-land[rx], max_land_height );
                        //vline( land_work, rx, max_land_height-land[rx], max_land_height, land_color );

                }

        }

}


void make_explosion( int x, int y, double walkx, double walky, int rad ) {

        int found = 0;
        while (found < max_expl && explr[ found ].alive){
                found++;
        }

        if ( found >= max_expl )
                return;

        explr[found].alive = true;
        explr[found].x = x;
        explr[found].y = y;
        explr[found].vx = x;
        explr[found].vy = y;
        explr[found].radius = rad;
        explr[found].max_rad = rad;
        explr[found].wx = walkx;
        explr[found].wy = walky;

}


void make_plane_explosion( int mx, int my ) {

        play_sample( snd[ snd_plane_crash ], 255, 128, 1000, false );
        make_explosion( mx-30, my, 0, 0, 35 );
        make_explosion( mx+30, my, 0, 0, 35 );
        make_explosion( mx, my-30, 0, 0, 35 );
        make_explosion( mx, my+30, 0, 0, 35 );

        make_explosion( mx-20, my-20, 0, 0, 35 );
        make_explosion( mx+20, my-20, 0, 0, 35 );
        make_explosion( mx-20, my+20, 0, 0, 35 );
        make_explosion( mx+20, my+20, 0, 0, 35 );

        make_explosion( mx, my, 0, 0, 40 );
        chop_land( mx, my, 40 );

}


void kill_player() {

        play_sample( snd[ snd_death ], 255, 128, 1000, false );
        //player.score -= 726*(player.mission+1);
        player.lives--;

        int mx = player.my_plane.realx;
        int my = player.my_plane.realy;
        for ( int r = 0; r< max_enemy; r++ )
                enemy[r].alive = false;
        make_plane_explosion( mx,my );

        player.my_plane.dead_phase = 50;
        player.my_plane.speed = 0;
        change_dx();

        /*
        for ( int r = 0; r< max_buildings; r++ )
        {
           build[r].rubble = false;
           build[r].has_plane = -1;
        }
        */

}


void check_hit() {

        int mx = player.my_plane.realx;
        int my = player.my_plane.realy;

        int ang = player.my_plane.angle;

        int plane_length = gen_plane_length;
        int pl1 = (int)(sqrt( (plane_length*plane_length)*2 ) );

        int x1 = (int)( mx+tcos[ ang ]* plane_length );
        int y1 = (int)( my+tsine[ang]*plane_length );

        int x2;
        int y2;
        int x3;
        int y3;

        if ( !player.flip ) {
                x2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
        }
        else {

                x2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

        if ( x1 < 0 )
                x1=0;
        if ( x1 >= max_land )
                x1 = max_land-1;

        if ( x2 < 0 )
                x2=0;
        if ( x2 >= max_land )
                x2 = max_land-1;

        if ( x3 < 0 )
                x3=0;
        if ( x3 >= max_land )
                x3 = max_land-1;

        bool before = build[0].alive;
        if ( y1 > screen_y - land[ x1 ] ||
                y2 > screen_y - land[ x2 ] ||
                y3 > screen_y - land[ x3 ] ||
                hit_building( x1, y1 ) ||
                hit_building( x2, y2 ) ||
                hit_building( x3, y3 ) )
                kill_player();                    /*then do this*/
        if ( !before )
                player.score -= 5000;

        //re_init_player();

}


int angl( int q ) {

        while ( q < 0 )
                q+=360;
        while ( q >= 360 )
                q %= 360;
        return q;

}


void change_dx() {

	/*
        while ( player.my_plane.angle < 0 )
                player.my_plane.angle += 360;

        player.my_plane.angle %= 360;
	*/

	player.my_plane.angle = angl( player.my_plane.angle );

        player.my_plane.dx = tcos[ player.my_plane.angle ] * player.my_plane.speed;
        player.my_plane.dy = tsine[ player.my_plane.angle ] * player.my_plane.speed;

}


void make_bomb( int x, int y, double wx, double wy, int wha ) {

        int found = 0;
        while ( bomb[found].alive && found < max_bomb )
                found++;

        if ( found >= max_bomb )
                return;
        play_sample( snd[ snd_bomb_away ], 255, 128, 1000, false );
        bomb[found].alive = true;
        bomb[found].vx = x;
        bomb[found].vy = y;
        bomb[found].wx = wx;
        bomb[found].wy = wy;
        bomb[found].vx += bomb[found].wx;
        bomb[found].vy += bomb[found].wy;
        bomb[found].mirv = wha;
        bomb[found].mirv_life = 65;

}


void make_shot( int x1, int y1, double wx, double wy, bool fall_ya ) {

        int found = 0;
        while ( shot[ found ].life > 0 && found < max_shot )
                found++;

        if ( found >= max_shot )
                return;

        play_sample( snd[ snd_shoot ], 128, 255, 1000, false );
        shot[found].vx = x1;
        shot[found].vy = y1;
        shot[found].wx = wx;
        shot[found].wy = wy;
        shot[found].life = 150+500*fall_ya;
        shot[found].fall = fall_ya;

}


bool player_hit_home() {
        int mx = player.my_plane.realx;
        int my = player.my_plane.realy;

        int ang = player.my_plane.angle;

        int plane_length = gen_plane_length;
        int pl1 = (int)(sqrt( (plane_length*plane_length)*2 ) );

        int x1 = (int)( mx+tcos[ ang ]* plane_length );
        int y1 = (int)( my+tsine[ang]*plane_length );

        int x2;
        int y2;
        int x3;
        int y3;

        if ( !player.flip ) {
                x2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
        }
        else {

                x2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

        if ( x1 < 0 )
                x1=0;
        if ( x1 >= max_land )
                x1 = max_land-1;

        if ( x2 < 0 )
                x2=0;
        if ( x2 >= max_land )
                x2 = max_land-1;

        if ( x3 < 0 )
                x3=0;
        if ( x3 >= max_land )
                x3 = max_land-1;

        if ( touch_building( x1, y1 ) == 0 ||
                touch_building( x2, y2 ) == 0 ||
                touch_building( x3, y3 ) == 0 )
                return true;
        else
                return false;

}


void home_plane() {

        if ( build[0].direction == true )
                player.my_plane.virtx = build[0].x+build[0].width+15;
        else
                player.my_plane.virtx = build[0].x - 15;

        player.my_plane.virty = build[0].y+build[0].size - 5;
        player.my_plane.stall = false;

        player.my_plane.realx = (int)player.my_plane.virtx;
        player.my_plane.realy = (int)player.my_plane.virty;

        player.my_plane.landed = true;
        player.home = false;

        if ( build[0].direction == false ) {
                player.my_plane.angle = 180;
                player.flip = true;
        }
        else {
                player.my_plane.angle = 0;
                player.flip = false;
        }

        player.my_plane.speed = 0;
        change_dx();

}


void player_move() {

        if ( player.real_score > player.next_level ) {

                //player.lives++;
                player.next_level += (player.mission+1) * 10000;

        }

        if ( player.my_plane.dead_phase > 0 ) {

                if ( (player.my_plane.dead_phase--) <= 1 )
                        install_player();

                return;

        }

        if ( key[ K_NORMALBOMB ] && player.my_plane.bomb_counter <= 0 && player.my_plane.bombs_left > 0 && player.my_plane.dead_phase <= 0 ) {
                player.my_plane.bombs_left--;
                int dist = 7;
                if (!player.flip)
                        make_bomb( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+270)%360]*dist), (int)(player.my_plane.realy+tsine[(player.my_plane.angle+270)%360]*dist),
                                player.my_plane.dx, player.my_plane.dy, 0 );
                else
                        make_bomb( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+90)%360]*dist), (int)(player.my_plane.realy+tsine[(player.my_plane.angle+90)%360]*dist),
                                player.my_plane.dx, player.my_plane.dy, 0 );

                player.my_plane.bomb_counter = 1;

        }
        if ( key[ K_MULTIBOMB ] && player.my_plane.bomb_counter <= 0 && player.my_plane.bombs_left > 1 && player.my_plane.dead_phase <= 0 ) {
                player.my_plane.bombs_left-=2;
                int dist = 7;
                if (!player.flip)
                        make_bomb( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+270)%360]*dist), (int)(player.my_plane.realy+tsine[(player.my_plane.angle+270)%360]*dist),
                                player.my_plane.dx, player.my_plane.dy, 1 );
                else
                        make_bomb( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+90)%360]*dist), (int)(player.my_plane.realy+tsine[(player.my_plane.angle+90)%360]*dist),
                                player.my_plane.dx, player.my_plane.dy, 1 );

                player.my_plane.bomb_counter = 1;

        }
        if ( key[ K_SHOTBOMB ] && player.my_plane.bomb_counter <= 0 && player.my_plane.bombs_left > 1 ) {
                player.my_plane.bombs_left-=2;
                int dist = 7;
                if (!player.flip)
                        make_bomb( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+270)%360]*dist), (int)(player.my_plane.realy+tsine[(player.my_plane.angle+270)%360]*dist),
                                player.my_plane.dx, player.my_plane.dy, 2 );
                else
                        make_bomb( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+90)%360]*dist), (int)(player.my_plane.realy+tsine[(player.my_plane.angle+90)%360]*dist),
                                player.my_plane.dx, player.my_plane.dy, 2 );

                player.my_plane.bomb_counter = 1;

        }
        if ( player.my_plane.bomb_counter > 0 )
                player.my_plane.bomb_counter++;
        if ( player.my_plane.bomb_counter > 50 )
                player.my_plane.bomb_counter = 0;

        if ( key[ K_FLIP ] && player.my_plane.flip_counter <= 0 ) {
                if ( player.flip )
                        player.flip = false;
                else
                        player.flip = true;

                player.my_plane.flip_counter = 1;

        }
        if ( player.my_plane.flip_counter > 0 )
                player.my_plane.flip_counter++;
        if ( player.my_plane.flip_counter > 20 )
                player.my_plane.flip_counter = 0;

        if ( (key[K_SHOOT] || (key[K_CANNON]&&player.my_plane.shots_left>4) ) && player.my_plane.shot_counter <= 0 && player.my_plane.speed > 0 && player.my_plane.shots_left > 0) {
                #define fmax 5
                player.my_plane.shots_left--;
                int angle_dep = 0;
                int ts = 10;
                if ( player.shoot_up )
                        player.shooting_ang+=2;
                else
                        player.shooting_ang-=2;
                if ( player.shooting_ang <= -fmax )
                        player.shoot_up = true;
                if ( player.shooting_ang >= fmax )
                        player.shoot_up = false;
                if ( player.shooting_ang < -fmax )
                        player.shooting_ang = -fmax;
                if ( player.shooting_ang > fmax )
                        player.shooting_ang = fmax;

                player.shx = tcos[ angl(player.my_plane.angle+player.shooting_ang)]*player.my_plane.speed;
                player.shy = tsine[angl(player.my_plane.angle+player.shooting_ang)]*player.my_plane.speed;

                if ( key[K_SHOOT] )
                        make_shot( (int)(player.my_plane.realx+tcos[(player.my_plane.angle+angle_dep) % 360]*ts),
                                (int)(player.my_plane.realy+tsine[( player.my_plane.angle+angle_dep) % 360 ]*ts),
                                player.shx*bullet_speed, player.shy*bullet_speed, false );
                else
                if ( key[K_CANNON]) {
                        player.my_plane.shots_left-=4;
                        #define plx player.my_plane.realx
                        #define ply player.my_plane.realy
                        #define pla player.my_plane.angle
                        make_shot( (int)(plx+tcos[(pla+angle_dep) % 360]*ts),
                                (int)(ply+tsine[( pla+angle_dep) % 360 ]*ts),
                                player.shx*bullet_speed, player.shy*bullet_speed, false );
                        make_shot( (int)(plx+tcos[(pla+angle_dep+3) % 360]*(ts-2)),
                                (int)(ply+tsine[(pla+angle_dep+3) % 360 ]*(ts-2)),
                                player.shx*bullet_speed, player.shy*bullet_speed, false );
                        make_shot( (int)(plx+tcos[angl(pla+angle_dep-3) % 360]*(ts-2)),
                                (int)(ply+tsine[angl(pla+angle_dep-3) % 360 ]*(ts-2)),
                                player.shx*bullet_speed, player.shy*bullet_speed, false );
                }
                player.my_plane.shot_counter = 1;

        }
        if ( player.my_plane.shot_counter > 0 )
                player.my_plane.shot_counter++;
        if ( player.my_plane.shot_counter > 10 - player.mission / 2 )
                player.my_plane.shot_counter = 0;

        if ( key[ K_HOME ] )
                player.home = true;

        if ( player.my_plane.stall )
                player.home = false;

        if ( player.home ) {
                if ( dist( player.my_plane.realx, player.my_plane.realy,
                        build[0].x+10, build[0].y+5 ) > 100 )
                        player.home_ang = gang( player.my_plane.realx, player.my_plane.realy,
                                build[0].x+10, build[0].y );
                else {
                        if ( !build[0].rubble )
                                player.home_ang = gang( player.my_plane.realx, player.my_plane.realy,
                                        build[0].x+10, build[0].y+build[0].size/3 );
                        else
                                player.home_ang = gang( player.my_plane.realx, player.my_plane.realy,
                                        build[0].x+10, build[0].y+build[0].size );
                }

                int &a1 = player.my_plane.angle;
                int a2 = player.home_ang;

                if ( a1 != a2 ) {

                        if ( ( a1 <= 90 && a2 >= 270 ) || ( a1 >= 270 && a2 <= 90 ) ) {

                                if ( a1 > 270 )
                                        a1 += 2;
                                else
                                        a1 -= 2;

                        }
                        else {
                                if ( a1 < a2 )
                                        a1 += 2;
                                else
                                        a1 -= 2;

                        }
                        if ( a1 == 0 )
                                a1 = 359;

                        change_dx();

                }

                if ( player_hit_home() ) {

                        player.my_plane.landed = true;
                        player.my_plane.stall = false;
                        home_plane();

                }

        }

        if ( player.my_plane.landed && player.my_plane.speed == 0 ) {

                if ( player.my_plane.fuel_left < max_fuel )
                        player.my_plane.fuel_left += 50;
                if ( player.my_plane.health < 50 && grand_clock % 4 == 0 )
                        player.my_plane.health++;
                if ( player.my_plane.bombs_left < max_bomb_left && grand_clock % 10 == 0)
                        player.my_plane.bombs_left++;
                if ( player.my_plane.shots_left < max_shot_left && grand_clock % 7 == 0)
                        player.my_plane.shots_left+=2;
                if ( player.my_plane.shots_left > max_shot_left )
                        player.my_plane.shots_left = max_shot_left;

        }

        if ( key[ K_TURNLEFT ] && player.my_plane.speed > 0 && player.my_plane.fuel_left > 0) {
                player.my_plane.landed = false;
                player.home = false;
                if ( !player.flip )
                        player.my_plane.angle += 2;
                else
                        player.my_plane.angle -= 2;

                change_dx();
        }

        if ( key[ K_TURNRIGHT ] && player.my_plane.speed > 0 && player.my_plane.fuel_left > 0 ) {
                player.my_plane.landed = false;
                player.home = false;
                if ( !player.flip )
                        player.my_plane.angle -= 2;
                else
                        player.my_plane.angle += 2;

                change_dx();
        }

        if ( player.my_plane.angle < 200 || player.my_plane.angle > 300 )
                player.my_plane.stall = false;

        if ( key[ K_ACCELERATE ] && player.my_plane.stall == false ) {
                player.home = false;
                player.my_plane.speed += 0.03;
                if ( player.my_plane.speed > mission[ player.mission ].max_player_speed )
                        player.my_plane.speed = mission[ player.mission ].max_player_speed;
                change_dx();
        }

        if ( key[ K_DECCELERATE ] && player.my_plane.stall == false ) {
                player.home = false;
                player.my_plane.speed -= 0.02;
                if ( player.my_plane.speed < 0 )
                        player.my_plane.speed = 0;
                change_dx();
        }

        if ( player.my_plane.speed < mission[ player.mission ].min_plane_speed && player.my_plane.landed == false && player.my_plane.stall == false ) {

                player.my_plane.stall = true;
                //player.my_plane.speed = 0;
                //player.my_plane.angle = 270;
                change_dx();

        }

        //if ( ( player.my_plane.speed > mission[ player.mission].min_plane_speed && player.my_plane.landed == false ) || player.my_plane.landed )
        //   player.my_plane.stall = false;

        player.my_plane.fuel_left -= (int)player.my_plane.speed;
        if ( player.my_plane.fuel_left < 0 )
                player.my_plane.fuel_left = 0;
        if ( player.my_plane.fuel_left <= 0 && player.my_plane.stall == false) {

                //player.my_plane.speed = 0;
                //change_dx();
                player.my_plane.stall = true;
                player.my_plane.fuel_left = 0;

        }

        if ( player.my_plane.stall ) {
                if ( player.my_plane.angle < 270 && player.my_plane.angle > 90 )
                        player.my_plane.angle += 1;
                if ( player.my_plane.angle > 270 || player.my_plane.angle < 90 )
                        player.my_plane.angle -= 1;
                if ( player.my_plane.angle < 0 )
                        player.my_plane.angle += 360;
                player.my_plane.speed += 0.04;
                change_dx();

        }

        player.my_plane.virtx += player.my_plane.dx;
        player.my_plane.realx = (int)player.my_plane.virtx;
        player.my_plane.virty += player.my_plane.dy;

        if ( player.my_plane.virty > 480 )
                player.my_plane.virty = 480;

        player.my_plane.realy = (int)player.my_plane.virty;

        /*
        if ( player.my_plane.dead_phase <= 0 )
           if ( touch_building( player.my_plane.realx, player.my_plane.realy ) == 0 )
              install_player();
        */

        if ( player.home )
        if ( player_hit_home() ) {
                player.my_plane.landed = true;
                player.my_plane.stall = false;
                home_plane();
        }

        if ( player.my_plane.dead_phase <= 0 )
                check_hit();

        player.my_plane.showy = player.my_plane.realy;
        if ( player.my_plane.showy < minimum )
                player.my_plane.showy = minimum;

        if ( player.my_plane.realx < 0 ) {

                player.my_plane.realx = 0;
                player.my_plane.angle = ( player.my_plane.angle + 180 ) % 360;
                change_dx();

        }

        if ( player.my_plane.realx >= max_land ) {

                player.my_plane.realx = max_land-1;
                player.my_plane.angle = ( player.my_plane.angle + 180 ) % 360;
                change_dx();

        }

        camera = player.my_plane.realx - screen_x / 2;
        player.my_plane.showx = screen_x / 2;

        if ( camera < 0 ) {

                camera = 0;
                player.my_plane.showx = player.my_plane.realx;

        }

        if ( camera > max_land - screen_x ) {

                camera = max_land - screen_x;
                player.my_plane.showx = player.my_plane.realx - camera;

        }

}


void bomb_move() {

        for ( int r = 0; r< max_bomb; r++ )
        if ( bomb[r].alive ) {

                bomb[r].vx += bomb[r].wx;
                bomb[r].vy += bomb[r].wy;
                bomb[r].wy += 0.02;
                bomb[r].x = (int)bomb[r].vx;
                bomb[r].y = (int)bomb[r].vy;
                if ( bomb[r].mirv!=0)
                        bomb[r].mirv_life--;
                if ( bomb[r].mirv_life<=0) {
                        bomb[r].alive = false;
                        make_explosion( bomb[r].x, bomb[r].y, 0, 0, 20 );
                        chop_land( bomb[r].x, bomb[r].y, 20 );
                        switch ( bomb[r].mirv ) {
                                case 1 :
                                        for ( int lp = 0; lp < 7; lp++ )
                                                make_bomb( (int)bomb[r].vx, (int)bomb[r].vy,
                                                        posneg()*(random()%3+1.2)*0.191, posneg()*(random()%2+1)*0.782, 0 );
                                        break;
                                case 2  :
                                        for ( int lp = 0; lp < 50; lp++ )
                                                make_shot( (int)bomb[r].vx, (int)bomb[r].vy,
                                                        posneg()*random_float(1), -random_float(4), true );
                                        break;
                        }
                }
                else
                if ( bomb[r].x < 0 || bomb[r].x >= max_land )
                        bomb[r].alive = false;
                else
                if ( hit_building( bomb[r].x, bomb[r].y ) ||
                bomb[r].y > screen_y - land[ bomb[r].x ] ) {

                        bomb[r].alive = false;
                        make_explosion( bomb[r].x, bomb[r].y, 0, 0, 20 );
                        chop_land( bomb[r].x, bomb[r].y, 20 );

                }
                else {

                        int ox = player.my_plane.realx;
                        int oy = player.my_plane.realy;

                        if ( dist( ox, oy, bomb[r].x, bomb[r].y ) <= 4 ) {
                                bomb[r].alive = false;
                                kill_player();
                        }

                }                                 //else

                if ( bomb[r].alive ) {

                        for ( int q = 0;q < max_enemy; q++ )
                        if ( enemy[q].alive ) {

                                int ox = enemy[q].my_plane.realx;
                                int oy = enemy[q].my_plane.realy;

                                if ( dist( ox, oy, bomb[r].x, bomb[r].y ) <= 4 ) {

                                        bomb[r].alive = false;
                                        enemy[q].alive = false;
                                        make_plane_explosion( ox, oy );

                                }

                        }                         //for if

                }                                 //if
                else
                        play_sample( snd[ snd_bomb_hit ], 255, 128,1000, false );

        }                                         // for loop if

}


bool collision_with_plane( int d ) {

        int sx = shot[d].x;
        int sy = shot[d].y;

        int x2, x3, y2, y3;

        int mx = player.my_plane.realx;
        int my = player.my_plane.realy;

        int ang = player.my_plane.angle;

        int plane_length = gen_plane_length;
        int pl1 = (int)(sqrt( (plane_length*plane_length)*2 ) );

        int x1 = (int)( mx+tcos[ ang ]* plane_length );
        int y1 = (int)( my+tsine[ang]*plane_length );

        if ( !player.flip ) {
                x2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
        }
        else {

                x2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

        if ( x1 < 0 )
                x1=0;
        if ( x1 >= max_land )
                x1 = max_land-1;

        if ( x2 < 0 )
                x2=0;
        if ( x2 >= max_land )
                x2 = max_land-1;

        if ( x3 < 0 )
                x3=0;
        if ( x3 >= max_land )
                x3 = max_land-1;

        if ( point_in_triangle( x1, y1, x2, y2, x3, y3, sx, sy ) && player.my_plane.dead_phase <= 0 ) {

                player.my_plane.health -= 10;
                if ( player.my_plane.health <= 0 )
                        kill_player();
                return true;

        }
        else {

                for ( int ds = 0; ds < max_enemy; ds++ )
                if ( enemy[ds].alive ) {

                        mx = enemy[ds].my_plane.realx;
                        my = enemy[ds].my_plane.realy;
                        ang = enemy[ds].my_plane.angle;
                        plane_length = gen_plane_length;
                        pl1 = (int)(sqrt( (plane_length*plane_length)*2 ) );

                        x1 = (int)( mx+tcos[ ang ]* plane_length );
                        y1 = (int)( my+tsine[ang]*plane_length );

                        if ( !enemy[ds].flip ) {
                                x2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                                y2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
                        }
                        else {

                                x2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                                y2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

                        }

                        if ( x1 < 0 )
                                x1=0;
                        if ( x1 >= max_land )
                                x1 = max_land-1;

                        if ( x2 < 0 )
                                x2=0;
                        if ( x2 >= max_land )
                                x2 = max_land-1;

                        if ( x3 < 0 )
                                x3=0;
                        if ( x3 >= max_land )
                                x3 = max_land-1;

                        if ( point_in_triangle( x1, y1, x2, y2, x3, y3, sx, sy ) ) {

                                enemy[ds].my_plane.health -= 10;
                                if ( enemy[ds].my_plane.health <= 0 ) {

                                        player.score += 100;
                                        player.real_score += 100;
                                        enemy[ds].alive = false;
                                        make_plane_explosion( enemy[ds].my_plane.realx, enemy[ds].my_plane.realy );
                                        //make_explosion( enemy[ds].my_plane.realx, enemy[ds].my_plane.realy, 0, 0, 20 );
                                        //chop_land( enemy[ds].my_plane.realx, enemy[ds].my_plane.realy, 20 );
                                        ds = max_enemy;
                                }

                                return true;

                        }

                }                                 //for

        }                                         //else

        return false;

}


bool triangle_touch( int ax1, int ay1, int ax2, int ay2, int ax3, int ay3, int bx1, int by1, int bx2, int by2, int bx3, int by3 ) {

        if ( line_hit( ax1, ay1, ax2, ay2, bx1, by1, bx2, by2 ) ||
                line_hit( ax1, ay1, ax3, ay3, bx1, by1, bx2, by2 ) ||
                line_hit( ax2, ay2, ax3, ay3, bx1, by1, bx2, by2 ) ||
                line_hit( ax1, ay1, ax2, ay2, bx1, by1, bx3, by3 ) ||
                line_hit( ax1, ay1, ax3, ay3, bx1, by1, bx3, by3 ) ||
                line_hit( ax2, ay2, ax3, ay3, bx1, by1, bx3, by3 ) ||
                line_hit( ax1, ay1, ax2, ay2, bx2, by2, bx3, by3 ) ||
                line_hit( ax1, ay1, ax3, ay3, bx2, by2, bx3, by3 ) ||
                line_hit( ax2, ay2, ax3, ay3, bx2, by2, bx3, by3 ) )
                return true;
        else
                return false;

}


void shot_move() {

        for ( int r = 0; r< max_shot; r++ )
        if ( shot[r].life > 0 ) {

                shot[r].vx += shot[r].wx;
                shot[r].vy += shot[r].wy;
                if ( shot[r].fall )
                        shot[r].wy += 0.08;
                shot[r].x = (int)shot[r].vx;
                shot[r].y = (int)shot[r].vy;

                if ( shot[r].x < 0 || shot[r].x >= max_land )
                        shot[r].life = 0;
                else {

                        if ( shot[r].y >= screen_y - land[ shot[r].x ] ) {
                                vline( land_work, shot[r].x, 0, max_land_height, 0 );
                                land[ shot[r].x ]--;
                                if ( land[shot[r].x] < 1 )
                                        land[shot[r].x] = 1;
                                draw_land_line( land_work, shot[r].x, max_land_height-land[shot[r].x], max_land_height );
                                shot[r].life = 0;
                        }
                        else
                        if ( touch_building( shot[r].x, shot[r].y ) >= 0 ) {

                                int qw = touch_building( shot[r].x, shot[r].y );
                                if ( qw >= 0 ) {

                                        shot[r].life = 0;
                                        build[qw].health -= 10;
                                        if ( build[qw].health <= 0 && build[qw].rubble == false) {
                                                play_sample( snd[ snd_destroy ], 255, 128, 1000, false );
                                                rubble_building( qw );
                                                int x1 = build[qw].x;
                                                int x2 = build[qw].x+build[qw].width;
                                                int y1 = build[qw].y;
                                                int y2 = build[qw].y+build[qw].size;
                                                explode_building( (x1+x2)/2, (y1+y2)/2, build[qw].kind );
                                                player.score += scoring( build[qw].kind );
                                                player.real_score += scoring( build[qw].kind );
                                                if ( qw == 0 )
                                                        player.score -= 5000;

                                        }         //build[qw].health

                                }                 //qw > 0

                        }                         //else if
                        else
                        if ( collision_with_plane( r ) )
                                shot[r].life = 0;
                }                                 //else

                if ( shot[r].life <= 0 )
                        make_explosion( shot[r].x, shot[r].y, 0, 0, 5 );

                shot[r].life--;
                if ( shot[r].life < 0 )
                        shot[r].life = 0;

        }                                         //for

}


void make_a_plane( int w ) {

        int found = 0;
        while (found < max_enemy && enemy[found].alive){
                found++;
        }

        if ( found >= max_enemy )
                return;

        build[w].has_plane = found;
        enemy[ found ].alive = true;

        if ( build[w].direction == true )
                enemy[ found ].my_plane.virtx = build[w].x+build[w].width+15;
        else
                enemy[ found ].my_plane.virtx = build[w].x - 15;

        enemy[ found ].my_plane.virty = build[w].y+build[w].size - 5;

        enemy[ found ].my_plane.realx = (int)enemy[ found ].my_plane.virtx;
        enemy[ found ].my_plane.realy = (int)enemy[ found ].my_plane.virty;

        enemy[ found ].my_plane.dead_phase = 0;
        enemy[ found ].my_plane.showx = 320;
        enemy[ found ].my_plane.showy = enemy[ found ].my_plane.realy;
        enemy[ found ].my_plane.health = 40;
        enemy[ found ].my_plane.landed = true;
        enemy[ found ].my_plane.stall = false;

        if ( build[w].direction == false ) {
                enemy[ found ].my_plane.angle = 180;
                enemy[ found ].flip = true;
        }
        else {
                enemy[ found ].my_plane.angle = 0;
                enemy[ found ].flip = false;
        }

        enemy[ found ].my_plane.speed = 0;
        enemy[ found ].my_plane.bombs_left = 10;
        enemy[ found ].my_plane.shots_left = 100;
        enemy[ found ].my_plane.fuel_left = max_fuel - 2000;
        enemy[ found ].my_plane.flip_counter = -1;
        enemy[ found ].my_plane.bomb_counter = -1;
        enemy[ found ].my_plane.shot_counter = -1;
        enemy[ found ].my_plane.dx = 0;
        enemy[ found ].my_plane.dy = 0;
        enemy[ found ].go_up = 200;
        enemy[ found ].my_plane.allegiance = 0;
        build[w].max_plane = 700;
        if ( build[w].rubble )
                build[w].max_plane = 1500;
        build[w].make_plane = 0;

}


void do_stuff_with_buildings() {

        for ( int r = 1; r< max_buildings; r++ )
        if ( build[r].alive ) {

                int rang = gang( (build[r].x+build[r].x+build[r].width) / 2, build[r].y, player.my_plane.realx, player.my_plane.realy );

                int q = player.mission * 5 / 3 + ( random() % 10 ) / 2;

                if ( q > 8 && random() % 15 == random() % 15 && rang < 180 && build[r].rubble == false && build[r].kind != cow && dist( player.my_plane.realx, player.my_plane.realy, (build[r].x*2+build[r].width)/2, build[r].y-1 ) < 300 && !player.my_plane.landed ) {
                        //these next 3 statements if to the else are jacked up
                        if ( build[r].kind == gun && random() % 5 == random() % 5 )
                                make_explosion( (build[r].x+build[r].x+build[r].width)/2, build[r].y-10, tcos[rang]*9.5, tsine[rang]*9.5, 45 );
                        else
                                make_shot( (build[r].x+build[r].x+build[r].width) /2 , build[r].y-1, tcos[rang]*4.5, tsine[rang]*4.5, false );

                }

                if ( build[r].kind == base ) {

                        if ( build[r].has_plane == -1 )
                                make_a_plane( r );
                        else
                        if ( enemy[ build[r].has_plane ].alive == false && build[r].make_plane > build[r].max_plane )
                                make_a_plane( r );
                        else
                        if ( build[r].max_plane != 0 && build[r].make_plane <= build[r].max_plane )
                                build[r].make_plane++;

                }

        }

}


void change_enemy_dx( int w ) {

        while ( enemy[w].my_plane.angle < 0 )
                enemy[w].my_plane.angle += 360;

        enemy[w].my_plane.angle %= 360;

        enemy[w].my_plane.dx = tcos[ enemy[w].my_plane.angle ] * enemy[w].my_plane.speed;
        enemy[w].my_plane.dy = tsine[ enemy[w].my_plane.angle ] * enemy[w].my_plane.speed;

}


void check_collisions( int gr ) {

        int x2, x3, y2, y3;

        int mx = enemy[gr].my_plane.realx;
        int my = enemy[gr].my_plane.realy;

        int ang = enemy[gr].my_plane.angle;

        int plane_length = gen_plane_length;
        int pl1 = (int)(sqrt( (plane_length*plane_length)*2 ) );

        int x1 = (int)( mx+tcos[ ang ]* plane_length );
        int y1 = (int)( my+tsine[ang]*plane_length );

        if ( !enemy[gr].flip ) {
                x2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
        }
        else {

                x2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                y2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                x3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                y3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

        if ( x1 < 0 )
                x1=0;
        if ( x1 >= max_land )
                x1 = max_land-1;

        if ( x2 < 0 )
                x2=0;
        if ( x2 >= max_land )
                x2 = max_land-1;

        if ( x3 < 0 )
                x3=0;
        if ( x3 >= max_land )
                x3 = max_land-1;

        //int gx = player.my_plane.realx;
        //int gy = player.my_plane.realy;
        /*if ( dist( x1, y1, gx, gy ) < 5 ||
             dist( x2, y2, gx, gy ) < 5 ||
             dist( x3, y3, gx, gy ) < 5 )
           {
        */

        mx = player.my_plane.realx;
        my = player.my_plane.realy;
        int ax1,ax2,ax3,ay1,ay2,ay3;

        ang = player.my_plane.angle;
        ax1 = (int)( mx+tcos[ ang ]* plane_length );
        ay1 = (int)( my+tsine[ang]*plane_length );

        if ( !player.flip ) {
                ax2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                ay2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                ax3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                ay3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
        }
        else {

                ax2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                ay2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                ax3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                ay3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

        }

        if ( ax1 < 0 )
                ax1=0;
        if ( ax1 >= max_land )
                ax1 = max_land-1;

        if ( ax2 < 0 )
                ax2=0;
        if ( ax2 >= max_land )
                ax2 = max_land-1;

        if ( ax3 < 0 )
                ax3=0;
        if ( ax3 >= max_land )
                ax3 = max_land-1;

        if (
                /*( point_in_triangle( ax1, ay1, ax2, ay2, ax3, ay3, x1, y1 ) ||
                        point_in_triangle( ax1, ay1, ax2, ay2, ax3, ay3, x2, y2 ) ||
                        point_in_triangle( ax1, ay1, ax2, ay2, ax3, ay3, x3, y3 ) )*/
                triangle_touch( ax1, ay1, ax2, ay2, ax3, ay3,
                x1, y1, x2, y2, x3, y3 )
        && player.my_plane.dead_phase <= 0 ) {
                //textprintf( screen, font, 1, 1, 31, "Player collided with enemy %d", gr );
                //repeat until( key[ KEY_A ] )
                enemy[gr].alive = false;
                make_plane_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy );
                //make_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 0, 0, 10 );
                //chop_land( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 10 );
                kill_player();

        }
        else
        if ( y1 >= screen_y - land[ x1 ] ||
                y2 >= screen_y - land[ x2 ] ||
        y3 >= screen_y - land[ x3 ] ) {

                enemy[gr].alive = false;
                make_plane_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy );
                //make_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 0, 0, 25 );
                //chop_land( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 25 );

        }
        else {

                int mk = touch_building( x1, y1 );
                if ( mk == -1 )
                        mk = touch_building( x2, y2 );
                if ( mk == -1 )
                        mk = touch_building( x3, y3 );
                if ( mk >= 0 ) {

                        enemy[gr].alive = false;
                        make_plane_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy );

                        player.score += scoring( build[ mk ].kind );
                        player.real_score += scoring( build[mk].kind );
                        //make_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 0, 0, 25 );
                        //chop_land( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 25 );
                        explode_building( build[mk].x, build[mk].y, build[mk].kind );
                        play_sample( snd[ snd_destroy ], 255, 128, 1000, false );
                        rubble_building( mk );

                }

        }                                         //else if

        if ( enemy[gr].alive ) {

                for ( int r = 0; r< max_enemy; r++ )
                if ( enemy[r].alive && gr != r ) {

                        mx = enemy[r].my_plane.realx;
                        my = enemy[r].my_plane.realy;
                        //int ax1,ax2,ax3,ay1,ay2,ay3;

                        ang = enemy[r].my_plane.angle;
                        ax1 = (int)( mx+tcos[ ang ]* plane_length );
                        ay1 = (int)( my+tsine[ang]*plane_length );

                        if ( !enemy[r].flip ) {
                                ax2 = (int)( mx + tcos[ (ang+angular_1) % 360 ] * pl1 );
                                ay2 = (int)( my + tsine[ (ang+angular_1) % 360 ] * pl1 );

                                ax3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                                ay3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );
                        }
                        else {

                                ax2 = (int)( mx + tcos[ (ang+angular_1+90) % 360 ] * pl1 );
                                ay2 = (int)( my + tsine[ (ang+angular_1+90) % 360 ] * pl1 );

                                ax3 = (int)( mx + tcos[ (ang+angular_2) % 360 ] * plane_length );
                                ay3 = (int)( my + tsine[ (ang+angular_2) % 360 ] * plane_length );

                        }

                        /*if ( point_in_triangle( ax1, ay1, ax2, ay2, ax3, ay3, x1, y1 ) ||
                               point_in_triangle( ax1, ay1, ax2, ay2, ax3, ay3, x2, y2 ) ||
                               point_in_triangle( ax1, ay1, ax2, ay2, ax3, ay3, x3, y3 ) )
                        */
                        if ( triangle_touch( ax1, ay1, ax2, ay2, ax3, ay3,
                        x1,  y1,  x2,  y2,  x3,  y3 ) ) {
                                //textprintf(screen, font, 1, 1, 31, "Enemy %d collided with enemy %d", gr, r );
                                //repeat until( key[KEY_A] )
                                enemy[gr].alive = false;
                                enemy[r].alive = false;
                                player.score += 100;
                                player.real_score += 100;
                                make_plane_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy );
                                make_plane_explosion( enemy[r].my_plane.realx, enemy[r].my_plane.realy );

                                //make_explosion( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 0, 0, 20 );
                                //chop_land( enemy[gr].my_plane.realx, enemy[gr].my_plane.realy, 20 );
                                //make_explosion( enemy[r].my_plane.realx, enemy[r].my_plane.realy, 0, 0, 20 );
                                //chop_land( enemy[r].my_plane.realx, enemy[r].my_plane.realy, 20 );

                                r = max_enemy;

                        }

                }                                 //for if

        }                                         //if

        if ( enemy[gr].alive == false ) {
                player.score += 100;
                player.real_score += 100;
        }

}


void enemy_move() {

        for ( int r = 0; r < max_enemy; r++ )
        if ( enemy[r].alive ) {

                enemy[r].my_plane.virtx += enemy[r].my_plane.dx;
                enemy[r].my_plane.virty += enemy[r].my_plane.dy;

                if ( enemy[r].my_plane.virtx < 0 )
                        enemy[r].my_plane.virtx = 0;

                if ( enemy[r].my_plane.virtx >= max_land )
                        enemy[r].my_plane.virtx = max_land-1;

                if ( enemy[r].my_plane.virty > 480 )
                        enemy[r].my_plane.virty = 480;

                enemy[r].my_plane.realx = (int)enemy[r].my_plane.virtx;
                enemy[r].my_plane.realy = (int)enemy[r].my_plane.virty;

                int x1, x2, x3, y1, y2, y3;
                get_triangle_points( x1, y1, x2, y2, x3, y3 );
                int ax = ( x1 + x2 + x3 ) / 3;
                int ay = ( y1 + y2 + y3 ) / 3;

                if ( dist( enemy[r].my_plane.realx, enemy[r].my_plane.realy,
                        ax, ay ) > 100 )
                        enemy[r].want_ang = gang( enemy[r].my_plane.realx,
                                enemy[r].my_plane.realy,
                                ax, ay );
                else
                        enemy[r].want_ang = gang( enemy[r].my_plane.realx,
                                enemy[r].my_plane.realy,
                                (int)(ax+tcos[player.my_plane.angle]*10),
                                (int)(ay+tsine[player.my_plane.angle]*10) );

                if ( enemy[r].go_up > 70 ) {
                        if ( enemy[r].flip )
                                enemy[r].want_ang = 180;
                        else
                                enemy[r].want_ang = 0;

                        enemy[r].go_up--;
                        enemy[r].my_plane.angle = enemy[r].want_ang;
                }
                else
                if ( enemy[r].go_up > 0 ) {

                        enemy[r].want_ang = 90;
                        enemy[r].go_up--;
                        enemy[r].my_plane.landed = false;

                }

                if ( enemy[r].go_up == 0 ) {
                        if ( enemy[r].my_plane.angle < 90 || enemy[r].my_plane.angle > 270 )
                                enemy[r].flip = false;
                        else
                                enemy[r].flip = true;
                }

                if ( enemy[r].my_plane.speed < mission[player.mission].max_plane_speed )
                        enemy[r].my_plane.speed += 0.01;

                int &a1 = enemy[r].my_plane.angle;
                int a2 = enemy[r].want_ang;

                int much = ( random() % mission[ player.mission ].enemy_int ) + mission[ player.mission ].min_enemy_int;
                if ( a1 != a2 ) {
                        if ( ( a1 <= 90 && a2 >= 270 ) || ( a1 >= 270 && a2 <= 90 ) ) {
                                if ( a1 > 270 )
                                        a1 += much;
                                else
                                        a1 -= much;
                        }
                        else {
                                if ( a1 < a2 )
                                        a1 += much;
                                else
                                        a1 -= much;
                        }

                }                                 //if

                enemy[r].my_plane.fuel_left -= (int)enemy[r].my_plane.speed;
                if ( enemy[r].my_plane.fuel_left <= 0 ) {

                        enemy[r].my_plane.fuel_left = 0;
                        enemy[r].my_plane.speed += 0.02;
                        if ( enemy[r].my_plane.angle < 270 && enemy[r].my_plane.angle > 90 )
                                enemy[r].my_plane.angle += 1;
                        if ( enemy[r].my_plane.angle > 270 && enemy[r].my_plane.angle < 90 )
                                enemy[r].my_plane.angle -= 1;
                        if ( enemy[r].my_plane.angle < 0 )
                                enemy[r].my_plane.angle += 360;

                }

                change_enemy_dx( r );

                if ( abs( a1-a2 ) < 10 && enemy[r].my_plane.shot_counter <= 0 && enemy[r].go_up <= 0 ) {
                        int ts = 10;
                        make_shot( (int)(enemy[r].my_plane.realx+tcos[enemy[r].my_plane.angle+1]*ts), (int)(enemy[r].my_plane.realy+tsine[enemy[r].my_plane.angle+1]*ts),
                                enemy[r].my_plane.dx*bullet_speed, enemy[r].my_plane.dy*bullet_speed, false );
                        enemy[r].my_plane.shot_counter = 1;
                }

                if ( enemy[r].my_plane.bomb_counter > 0 )
                        enemy[r].my_plane.bomb_counter++;
                if ( enemy[r].my_plane.bomb_counter > 50 )
                        enemy[r].my_plane.bomb_counter = 0;

                if ( enemy[r].my_plane.shot_counter > 0 )
                        enemy[r].my_plane.shot_counter++;
                if ( enemy[r].my_plane.shot_counter > 20 )
                        enemy[r].my_plane.shot_counter = 0;

                check_collisions( r );

        }                                         //for if

}


void debug() {

        textprintf_ex( screen, font, 1, 1, 31, -1, "Debugging" );
        textprintf_ex( screen, font, 1, 10, 31, -1, "X %d Y %d", player.my_plane.realx, player.my_plane.realy );
        textprintf_ex( screen, font, 1, 20, 31, -1, "Camera %d", camera );

        readkey();

}


bool finished() {

	if ( player.lives <= 0 ) return true;
	if ( key[K_QUIT] ) return true;
        for ( int r = 1; r< max_buildings; r++ )
                if ( build[r].alive && !build[r].rubble )
			return false;
        for ( int r = 0; r< max_expl; r++ )
                if ( explr[r].alive )
			return false;
	return true;
}


void re_init_all() {

        clear( land_work );
        clear( work );
        set_misc();
        install_land();
        re_init_player();
        set_colors();

}


void run_game() {

        grand_clock = 0;
        speed_counter = 0;
        player.lives = mission[ player.mission ].starting_lives;

        PALETTE pl;
        get_palette( pl );
        player_move();
        draw_environment();
        fade_out(64);
        fade_in( pl, 2 );
        do {
                grand_clock++;

		if ( speed_counter > 1 ) speed_counter = 1;
                bool dr = false;
                while ( speed_counter ) {
                        player_move();
                        enemy_move();
                        do_stuff_with_buildings();
                        bomb_move();
                        shot_move();              //do this twice!
                        shot_move();              //here it is, the second time
                        speed_counter--;
                        dr = true;
                }

                if ( dr ) draw_environment();

		if ( key[ K_SCREENSHOT ] ){
        		PALETTE xx;
        		get_palette( xx );
			save_bitmap( "screenshot.bmp", screen, xx );
		}

		while ( speed_counter == 0 ) YIELD();

        } while ( !finished() );
        player.mission++;
        if ( player.lives != 0 )
                player.lives = mission[ player.mission ].starting_lives;

}


void get_key( const char * ls, int & keyx, int & cy) {
        textprintf_ex(screen,font,20,cy,31, -1, "%s",ls);
        keyx = readkey() >> 8;
        textprintf_ex(screen,font,20+text_length(font,ls),cy,31, -1, "%c",scancode_to_ascii(keyx) );
        cy += 10;
}


void change_keys() {

        clear_to_color( screen, 24 );
        int cy = 280;
        textprintf_ex(screen,font,20,cy,31, -1, "Keys" );cy+=10;

        get_key( "Accelerate:", K_ACCELERATE, cy );
        get_key( "Deccelerate:", K_DECCELERATE, cy );
        get_key( "Rotate Left:", K_TURNLEFT, cy );
        get_key( "Rotate Right:", K_TURNRIGHT, cy );
        get_key( "Flip Ship:", K_FLIP, cy );
        get_key( "Bomb:", K_NORMALBOMB, cy );
        get_key( "Mirv Bomb:", K_MULTIBOMB, cy );
        get_key( "Death Head Bomb:", K_SHOTBOMB, cy );
        get_key( "Shoot:", K_SHOOT, cy );
        get_key( "Cannon:", K_CANNON, cy );
        get_key( "Go Home:", K_HOME, cy );

}


void show_mission() {

        bool quit = false;

        while ( !quit ) {
                clear( screen );
                for ( int r = 0; r< screen_x - 1; r++ )
                        vline( screen, r, 0, screen_y - 1, 24 );

                textprintf_ex( screen, font, 310, 240, 31, -1, "MISSION %d", player.mission+1 );

                int cy = 280;
                textprintf_ex( screen, font, 20, cy, 31, -1, "KEYS" );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Accelerate", scancode_to_ascii(K_ACCELERATE) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Deccelerate", scancode_to_ascii(K_DECCELERATE) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Rotate Left", scancode_to_ascii(K_TURNLEFT) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Rotate Right", scancode_to_ascii(K_TURNRIGHT) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Flip Ship", scancode_to_ascii(K_FLIP) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Bomb", scancode_to_ascii(K_NORMALBOMB) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Mirv Bomb", scancode_to_ascii(K_MULTIBOMB) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Death Head Bomb", scancode_to_ascii(K_SHOTBOMB) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Shoot. Unless changed, is SPACEBAR.", scancode_to_ascii(K_SHOOT) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Cannon", scancode_to_ascii(K_CANNON) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "%c:Go Home", scancode_to_ascii(K_HOME) );cy+=10;
                textprintf_ex( screen, font, 20, cy, 31, -1, "E: Edit keys" ); cy += 10;
                textprintf_ex( screen,font, 20, cy, 31, -1, "Q: Quit game" ); cy += 10;

                textprintf_ex( screen, font, 20, 460, 31, -1, "Made by Jon Rafkind" );

                clear_keybuf();
                readkey();
                quit = true;
                if ( key[K_EDIT] ) {
                        quit = false;
                        change_keys();
                }
        }

}


void show_final_score() {

        clear( screen );
        clear( work );

	char * str = int2str( player.score );
        textprintf_ex( screen, font, 300, 236, 39, -1, "FINAL SCORE %s", str );
	delete[] str;

        clear_keybuf();
        readkey();

}


void change_background() {
}


void inc_speed_counter() {
        speed_counter++;
}


END_OF_FUNCTION( inc_speed_counter );

int main() {

        first_init();
        LOCK_VARIABLE( speed_counter );
        LOCK_FUNCTION( (void *)inc_speed_counter );
        install_int_ex( inc_speed_counter, MSEC_TO_TIMER( GAME_SPEED ) );

        do {

                if ( !key[K_QUIT] )
                        show_mission();

                run_game();
                ultra_color = 0;

                if ( !key[K_QUIT] && player.mission < max_missions )
                        re_init_all();

                if ( player.mission >= max_missions || player.lives <= 0 )
                        minor_init();

        } while ( !key[ K_QUIT ] && player.mission < max_missions && player.lives > 0 );

        show_final_score();

        for ( int q = 0; q < max_sample; q++ )
                destroy_sample( snd[q] );
        destroy_bitmap( work );
        destroy_bitmap( land_work );

        fade_out( 10 );
        exit(1);

}


END_OF_MAIN();
