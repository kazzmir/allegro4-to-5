#ifndef _allegro_4_base_h
#define _allegro_4_base_h

#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>
#include <time.h>
#include <stdarg.h>

#define AL_FUNC(type, name, args)               type name args
#ifdef AL_CONST
#undef AL_CONST
#endif
#define AL_CONST const

#define AL_VAR(type, name) extern type name
#ifndef AL_INLINE
#define AL_INLINE(type, name, args, code)       inline type name args code;
#endif
#define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#define AL_FUNCPTR(type, name, args)            extern type (*name) args
#define AL_METHOD(type, name, args)             type (*name) args
#define AL_ARRAY(type, name)                    extern type name[]
#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))
#define ZERO_SIZE_ARRAY(type, name)  type name[]
#define LOCK_FUNCTION(_)
#define LOCK_VARIABLE(_)
#define END_OF_FUNCTION(_)

extern int * allegro_errno;

#ifndef ALLEGRO_VERSION
#define ALLEGRO_VERSION 4
#define ALLEGRO_SUB_VERSION 4
#define ALLEGRO_WIP_VERSION 0
#endif

#define ALLEGRO_NO_ASM

#define AL_PI        3.14159265358979323846

#define ASSERT(x)

#define _AL_SINCOS(x, s, c)  do { (c) = cos(x); (s) = sin(x); } while (0)

#ifndef TRUE 
   #define TRUE         -1
   #define FALSE        0
#endif

#endif
