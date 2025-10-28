#ifdef _WIN32
   #define AL_INLINE(type, name, args, code) __declspec(dllexport) type name args code
#else
   #define AL_INLINE(type, name, args, code) extern inline type name args code
#endif

#include "allegro.h"
