/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Fixed point math inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FMATHS_INL
#define ALLEGRO_FMATHS_INL

// #define ALLEGRO_IMPORT_MATH_ASM
// #include "asm.inl"
//#undef ALLEGRO_IMPORT_MATH_ASM

#ifdef __cplusplus
   extern "C" {
#endif


/* ftofix and fixtof are used in generic C versions of fixmul and fixdiv */
AL_FUNC(fixed, ftofix, (double x));
       /*
AL_INLINE(fixed, ftofix, (double x),
{
   if (x > 32767.0) {
      *allegro_errno = ERANGE;
      return 0x7FFFFFFF;
   }

   if (x < -32767.0) {
      *allegro_errno = ERANGE;
      return -0x7FFFFFFF;
   }

   return (fixed)(x * 65536.0 + (x < 0 ? -0.5 : 0.5));
})
*/


AL_FUNC(double, fixtof, (fixed x));
/*
AL_INLINE(double, fixtof, (fixed x),
{
   return (double)x / 65536.0;
})
*/


#ifdef ALLEGRO_NO_ASM

/* use generic C versions */

AL_FUNC(fixed, fixadd, (fixed x, fixed y));
/*
AL_INLINE(fixed, fixadd, (fixed x, fixed y),
{
   fixed result = x + y;

   if (result >= 0) {
      if ((x < 0) && (y < 0)) {
         *allegro_errno = ERANGE;
         return -0x7FFFFFFF;
      }
      else
         return result;
   }
   else {
      if ((x > 0) && (y > 0)) {
         *allegro_errno = ERANGE;
         return 0x7FFFFFFF;
      }
      else
         return result;
   }
})
*/


AL_FUNC(fixed, fixsub, (fixed x, fixed y));
/*
AL_INLINE(fixed, fixsub, (fixed x, fixed y),
{
   fixed result = x - y;

   if (result >= 0) {
      if ((x < 0) && (y > 0)) {
         *allegro_errno = ERANGE;
         return -0x7FFFFFFF;
      }
      else
         return result;
   }
   else {
      if ((x > 0) && (y < 0)) {
         *allegro_errno = ERANGE;
         return 0x7FFFFFFF;
      }
      else
         return result;
   }
})
*/


/* In benchmarks conducted circa May 2005 we found that, in the main:
 * - IA32 machines performed faster with one implementation;
 * - AMD64 and G4 machines performed faster with another implementation.
 *
 * Benchmarks were mainly done with differing versions of gcc.
 * Results varied with other compilers, optimisation levels, etc.
 * so this is not optimal, though a tenable compromise.
 *
 * Note that the following implementation are NOT what were benchmarked.
 * We had forgotten to put in overflow detection in those versions.
 * If you don't need overflow detection then previous versions in the
 * CVS tree might be worth looking at.
 *
 * PS. Don't move the #ifs inside the AL_INLINE; BCC doesn't like it.
 */
AL_FUNC(fixed, fixmul, (fixed x, fixed y));
/*
#if (defined ALLEGRO_I386) || (!defined LONG_LONG)
   AL_INLINE(fixed, fixmul, (fixed x, fixed y),
   {
      return ftofix(fixtof(x) * fixtof(y));
   })
#else
   AL_INLINE(fixed, fixmul, (fixed x, fixed y),
   {
      LONG_LONG lx = x;
      LONG_LONG ly = y;
      LONG_LONG lres = (lx*ly);

      if (lres > 0x7FFFFFFF0000LL) {
	 *allegro_errno = ERANGE;
	 return 0x7FFFFFFF;
      }
      else if (lres < -0x7FFFFFFF0000LL) {
	 *allegro_errno = ERANGE;
	 return 0x80000000;
      }
      else {
	 int res = lres >> 16;
	 return res;
      }
   })
#endif	*/    /* fixmul() C implementations */


AL_FUNC(fixed, fixdiv, (fixed x, fixed y));
/*
AL_INLINE(fixed, fixdiv, (fixed x, fixed y),
{
   if (y == 0) {
      *allegro_errno = ERANGE;
      return (x < 0) ? -0x7FFFFFFF : 0x7FFFFFFF;
   }
   else
      return ftofix(fixtof(x) / fixtof(y));
})
*/


AL_FUNC(int, fixfloor, (fixed x));

#if 0
AL_INLINE(int, fixfloor, (fixed x),
{
   /* (x >> 16) is not portable */
   if (x >= 0)
      return (x >> 16);
   else
      return ~((~x) >> 16);
})
#endif


AL_FUNC(int, fixceil, (fixed x));
/*
AL_INLINE(int, fixceil, (fixed x),
{
   if (x > 0x7FFF0000) {
      *allegro_errno = ERANGE;
      return 0x7FFF;
   }

   return fixfloor(x + 0xFFFF);
})
*/

#endif      /* C vs. inline asm */


AL_FUNC(fixed, itofix, (int x));
/*
AL_INLINE(fixed, itofix, (int x),
{
   return x << 16;
})
*/


AL_FUNC(int, fixtoi, (fixed x));
/*
AL_INLINE(int, fixtoi, (fixed x),
{
   return fixfloor(x) + ((x & 0x8000) >> 15);
})
*/


AL_FUNC(fixed, fixcos, (fixed x));
/*
AL_INLINE(fixed, fixcos, (fixed x),
{
   return _cos_tbl[((x + 0x4000) >> 15) & 0x1FF];
})
*/

AL_FUNC(fixed, fixsin, (fixed x));

/*
AL_INLINE(fixed, fixsin, (fixed x),
{
   return _cos_tbl[((x - 0x400000 + 0x4000) >> 15) & 0x1FF];
})
*/


AL_FUNC(fixed, fixtan, (fixed x));
/*
AL_INLINE(fixed, fixtan, (fixed x),
{
   return _tan_tbl[((x + 0x4000) >> 15) & 0xFF];
})
*/

AL_FUNC(fixed, fixacos, (fixed x));

    /*
AL_INLINE(fixed, fixacos, (fixed x),
{
   if ((x < -65536) || (x > 65536)) {
      *allegro_errno = EDOM;
      return 0;
   }

   return _acos_tbl[(x+65536+127)>>8];
})
*/


AL_FUNC(fixed, fixasin, (fixed x));
/*
AL_INLINE(fixed, fixasin, (fixed x),
{
   if ((x < -65536) || (x > 65536)) {
      *allegro_errno = EDOM;
      return 0;
   }

   return 0x00400000 - _acos_tbl[(x+65536+127)>>8];
})
*/

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FMATHS_INL */


