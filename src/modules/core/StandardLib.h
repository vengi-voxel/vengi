/**
 * @file
 */

#pragma once

#include <SDL_stdinc.h>

#ifndef core_malloc
#define core_malloc SDL_malloc
#endif

#ifndef core_realloc
#define core_realloc SDL_realloc
#endif

#ifndef core_free
#define core_free SDL_free
#endif

#ifndef core_memset
#define core_memset SDL_memset
#endif

#ifndef core_memcpy
#define core_memcpy SDL_memcpy
#endif

#ifndef core_memcmp
#define core_memcmp SDL_memcmp
#endif

#ifndef core_zero
#define core_zero SDL_zero
#endif

#ifndef core_zerop
#define core_zerop SDL_zerop
#endif
