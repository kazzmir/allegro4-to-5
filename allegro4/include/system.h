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
 *      System level: initialization, cleanup, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_SYSTEM_H
#define ALLEGRO_SYSTEM_H

#include "base.h"
#include "unicode.h"
#include "config.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct RGB;
struct BITMAP;
struct GFX_VTABLE;
struct GFX_MODE;

#define ALLEGRO_ERROR_SIZE 256

AL_ARRAY(char, allegro_id);
AL_ARRAY(char, allegro_error);

#define OSTYPE_UNKNOWN     0
#define OSTYPE_WIN3        AL_ID('W','I','N','3')
#define OSTYPE_WIN95       AL_ID('W','9','5',' ')
#define OSTYPE_WIN98       AL_ID('W','9','8',' ')
#define OSTYPE_WINME       AL_ID('W','M','E',' ')
#define OSTYPE_WINNT       AL_ID('W','N','T',' ')
#define OSTYPE_WIN2000     AL_ID('W','2','K',' ')
#define OSTYPE_WINXP       AL_ID('W','X','P',' ')
#define OSTYPE_WIN2003     AL_ID('W','2','K','3')
#define OSTYPE_WINVISTA    AL_ID('W','V','S','T')
#define OSTYPE_WIN7        AL_ID('W','I','N','7')
#define OSTYPE_OS2         AL_ID('O','S','2',' ')
#define OSTYPE_WARP        AL_ID('W','A','R','P')
#define OSTYPE_DOSEMU      AL_ID('D','E','M','U')
#define OSTYPE_OPENDOS     AL_ID('O','D','O','S')
#define OSTYPE_LINUX       AL_ID('T','U','X',' ')
#define OSTYPE_SUNOS       AL_ID('S','U','N',' ')
#define OSTYPE_FREEBSD     AL_ID('F','B','S','D')
#define OSTYPE_NETBSD      AL_ID('N','B','S','D')
#define OSTYPE_OPENBSD     AL_ID('O','B','S','D')
#define OSTYPE_IRIX        AL_ID('I','R','I','X')
#define OSTYPE_DARWIN      AL_ID('D','A','R','W')
#define OSTYPE_QNX         AL_ID('Q','N','X',' ')
#define OSTYPE_UNIX        AL_ID('U','N','I','X')
#define OSTYPE_BEOS        AL_ID('B','E','O','S')
#define OSTYPE_HAIKU       AL_ID('H','A','I','K')
#define OSTYPE_MACOS       AL_ID('M','A','C',' ')
#define OSTYPE_MACOSX      AL_ID('M','A','C','X')
#define OSTYPE_PSP         AL_ID('K','P','S','P')

AL_VAR(int, os_type);
AL_VAR(int, os_version);
AL_VAR(int, os_revision);
AL_VAR(int, os_multitasking);

#define SYSTEM_AUTODETECT  0
#define SYSTEM_NONE        AL_ID('N','O','N','E')

#define MAKE_VERSION(a, b, c) (((a)<<16)|((b)<<8)|(c))

AL_FUNC(int, _install_allegro_version_check, (int system_id, int *errno_ptr,
   AL_METHOD(int, atexit_ptr, (AL_METHOD(void, func, (void)))), int version));

AL_FUNC(int, install_allegro, (int system_id, int *errno_ptr,
   AL_METHOD(int, atexit_ptr, (AL_METHOD(void, func, (void))))));

#define allegro_init()  _install_allegro_version_check(SYSTEM_AUTODETECT, &errno, \
   (int (*)(void (*)(void)))atexit, \
   MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION))

AL_FUNC(void, allegro_exit, (void));

AL_PRINTFUNC(void, allegro_message, (AL_CONST char *msg, ...), 1, 2);
AL_FUNC(void, get_executable_name, (char *output, int size));
AL_FUNC(int, set_close_button_callback, (AL_METHOD(void, proc, (void))));


AL_FUNC(void, check_cpu, (void));

/* CPU Capabilities flags for x86 capable chips */
#define CPU_ID       0x0001
#define CPU_FPU      0x0002
#define CPU_MMX      0x0004
#define CPU_MMXPLUS  0x0008
#define CPU_SSE      0x0010
#define CPU_SSE2     0x0020
#define CPU_3DNOW    0x0040
#define CPU_ENH3DNOW 0x0080
#define CPU_CMOV     0x0100
#define CPU_AMD64    0x0200
#define CPU_IA64     0x0400
#define CPU_SSE3     0x0800
#define CPU_SSSE3    0x1000
#define CPU_SSE41    0x2000
#define CPU_SSE42    0x4000

/* CPU families - PC */
#define CPU_FAMILY_UNKNOWN  0
#define CPU_FAMILY_I386     3
#define CPU_FAMILY_I486     4
#define CPU_FAMILY_I586     5
#define CPU_FAMILY_I686     6
#define CPU_FAMILY_ITANIUM  7
#define CPU_FAMILY_EXTENDED 15
/* CPUID only returns 15 bits, we need extra information from the CPU */
/*  model to identify Pentium IV, Xeon and Athlon 64 processors. */

/* CPU families - Power PC */
#define CPU_FAMILY_POWERPC  18

/* CPU models - PC */
/* 486 */
#define CPU_MODEL_I486DX    0
#define CPU_MODEL_I486DX50  1
#define CPU_MODEL_I486SX    2
#define CPU_MODEL_I487SX    3
#define CPU_MODEL_I486SL    4
#define CPU_MODEL_I486SX2   5
#define CPU_MODEL_I486DX2   7
#define CPU_MODEL_I486DX4   8

/* Intel/586 */
#define CPU_MODEL_PENTIUM              1
#define CPU_MODEL_PENTIUMP54C          2
#define CPU_MODEL_PENTIUMOVERDRIVE     3
#define CPU_MODEL_PENTIUMOVERDRIVEDX4  4
#define CPU_MODEL_CYRIX                14
#define CPU_MODEL_UNKNOWN              15

/* AMD/586 */
#define CPU_MODEL_K5                   0
#define CPU_MODEL_K6                   6

/* Intel/686 */
#define CPU_MODEL_PENTIUMPROA          0
#define CPU_MODEL_PENTIUMPRO           1
#define CPU_MODEL_PENTIUMIIKLAMATH     3
#define CPU_MODEL_PENTIUMII            5
#define CPU_MODEL_CELERON              6
#define CPU_MODEL_PENTIUMIIIKATMAI     7
#define CPU_MODEL_PENTIUMIIICOPPERMINE 8
#define CPU_MODEL_PENTIUMIIIMOBILE     9

/* AMD/686 */
#define CPU_MODEL_ATHLON               2
#define CPU_MODEL_DURON                3


/* Information when CPU_FAMILY is 15 */
#define CPU_MODEL_PENTIUMIV            0
#define CPU_MODEL_XEON                 2

#define CPU_MODEL_ATHLON64             4
#define CPU_MODEL_OPTERON              5

/* Information for Power PC processors */
/* these defines are taken from <mach-o/machine.h> */
#define CPU_MODEL_POWERPC_601	       1
#define CPU_MODEL_POWERPC_602	       2
#define CPU_MODEL_POWERPC_603	       3
#define CPU_MODEL_POWERPC_603e	       4
#define CPU_MODEL_POWERPC_603ev	       5
#define CPU_MODEL_POWERPC_604	       6
#define CPU_MODEL_POWERPC_604e	       7
#define CPU_MODEL_POWERPC_620	       8
#define CPU_MODEL_POWERPC_750	       9
#define CPU_MODEL_POWERPC_7400	       10
#define CPU_MODEL_POWERPC_7450	       11

AL_ARRAY(char, cpu_vendor);
AL_VAR(int, cpu_family);
AL_VAR(int, cpu_model);
AL_VAR(int, cpu_capabilities);


typedef struct SYSTEM_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
} SYSTEM_DRIVER;


AL_VAR(SYSTEM_DRIVER, system_none);
AL_VAR(SYSTEM_DRIVER *, system_driver);


#ifdef __cplusplus
   }
#endif


#endif          /* ifndef ALLEGRO_SYSTEM_H */
