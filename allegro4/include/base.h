#ifndef _allegro_4_base_h
#define _allegro_4_base_h

#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>
#include <time.h>
#include <stdarg.h>

#include "internal/alconfig.h"

#ifdef ALLEGRO_WINDOWS
   #ifdef ALLEGRO_NO_MAGIC_MAIN
      #define main _mangled_main
      #undef END_OF_MAIN

      #ifdef __cplusplus
extern "C" int __stdcall WinMain(void *hInst, void *hPrev, char *Cmd, int nShow);
      #endif

      #define END_OF_MAIN()                                                                        \
                                                                                                   \
         int __stdcall WinMain(void *hInst, void *hPrev, char *Cmd, int nShow) {                   \
            return _WinMain((void *)_mangled_main, hInst, hPrev, Cmd, nShow);                      \
         }
   #else
      #define END_OF_MAIN()
   #endif

   #undef _AL_DLL
   #if !defined(ALLEGRO425_STATICLINK)
      #ifdef ALLEGRO425_SOURCE
         #define _AL_DLL __declspec(dllexport)
      #else
         #define _AL_DLL __declspec(dllimport)
         #undef AL_INLINE
         #define AL_INLINE(type, name, args, code) inline type name args code
      #endif
   #else
      #define _AL_DLL
   #endif
#else
   #define END_OF_MAIN()
   #define _AL_DLL
#endif

#define AL_FUNC(type, name, args)               _AL_DLL type name args
#ifdef AL_CONST
#undef AL_CONST
#endif
#define AL_CONST const

#define AL_VAR(type, name) extern _AL_DLL type name
#ifndef AL_INLINE
#define AL_INLINE(type, name, args, code)       inline type name args code
#endif
#ifdef ALLEGRO_MSVC
#define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args)
#else
#define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#endif
#define AL_FUNCPTR(type, name, args)            extern _AL_DLL type (*name) args
#define AL_ARRAY(type, name)                    extern _AL_DLL type name[]
#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))
#define ZERO_SIZE_ARRAY(type, name)  type name[]

extern int * allegro_errno;

#ifndef ALLEGRO_VERSION
#define ALLEGRO_VERSION 4
#define ALLEGRO_SUB_VERSION 4
#define ALLEGRO_WIP_VERSION 0
#endif

#define ALLEGRO_NO_ASM

#define AL_PI        3.14159265358979323846

#define ASSERT(x)
#define TRACE(...)

#define _AL_SINCOS(x, s, c)  do { (c) = cos(x); (s) = sin(x); } while (0)

#ifndef TRUE 
   #define TRUE         -1
   #define FALSE        0
#endif

#endif
