#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _MSC_VER
   #define popen _popen
   #define pclose _pclose
#endif

#define FNAME "poly.png"

static int pts[11 * 2];

static void make_poly(int center_x, int center_y, int verts)
{
   for (int i = 0; i < verts; i++) {
      float angle = - (i << 1) * AL_PI / verts;
      pts[i * 2 + 0] = center_x + (int)(30.0f * cosf(angle));
      pts[i * 2 + 1] = center_y + (int)(30.0f * sinf(angle));
   }
}

static int render() {
   // Create a truecolor 32-bit off-screen bitmap
   set_color_depth(32);
   BITMAP* bmp = create_bitmap_ex(32, 320, 240);
   if (!bmp) {
      fprintf(stderr, "create_bitmap_ex() failed\n");
      return 1;
   }

   // Clear bitmap to blue
   int blue = makecol(0, 0, 255);
   clear_to_color(bmp, blue);

   // Green color for the polygons
   int green = makecol(0, 160, 0);

   int k = 3;
   for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++, k++) {
         make_poly(65 + i * 90, 45 + j * 70, k);
         polygon(bmp, k, pts, green);
      }

   save_bitmap(FNAME, bmp, NULL);
   destroy_bitmap(bmp);

   return 0;
}

static int check() {
   // return system("exiftool -if \"$ImageDataHash eq '72e18ab16e291c2444141998c0137c95'\" poly.png");
   FILE* fp;
   int ret = 2;
   fp = popen("magick identify -format %# " FNAME, "r");
   if (fp == NULL) {
      perror("popen");
      return 1;
   }
   char buf[1024];
   while (fgets(buf, sizeof(buf), fp) != NULL) {
      if (strcmp(buf, "48aa021832142bd29f1beb04becd8b4c8b1d4f9ce606903e35ade48fa202e613") == 0) ret = 0;
   }
   pclose(fp);
   return ret;
}

int main(int argc, char* argv[])
{
   int ret = 0;
   if (allegro_init() != 0) {
      fprintf(stderr, "allegro_init() failed\n");
      return 1;
   }

   // Don't create a window, use default RGB shifts
   // set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0);

   ret = render();

   if (argc > 1 && strcmp(argv[1], "--check") == 0)
      ret = check();

   return ret;
}
END_OF_MAIN();
