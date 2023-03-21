/**
 * @file
 */

#pragma once

#include <string.h>

#ifndef SDLCALL
#if defined(__WIN32__) && !defined(__GNUC__)
#define SDLCALL __cdecl
#else
#define SDLCALL
#endif
#endif

extern "C" void SDLCALL SDL_SIMDFree(void *ptr);
extern "C" void *SDLCALL SDL_SIMDAlloc(const size_t len);
extern "C" void *SDLCALL SDL_malloc(size_t size);
extern "C" void *SDLCALL SDL_realloc(void *mem, size_t size);
extern "C" void SDLCALL SDL_free(void *mem);
extern "C" void *SDLCALL SDL_memset(void *dst, int c, size_t len);
extern "C" void *SDLCALL SDL_memcpy(void *dst, const void *src, size_t len);
extern "C" int SDLCALL SDL_memcmp(const void *s1, const void *s2, size_t len);

#ifndef core_malloc
#define core_malloc SDL_malloc
#endif

#ifndef core_realloc
#define core_realloc SDL_realloc
#endif

#ifndef core_free
#define core_free SDL_free
#endif

#ifndef core_aligned_malloc
#define core_aligned_malloc SDL_SIMDAlloc
#endif

#ifndef core_aligned_free
#define core_aligned_free SDL_SIMDFree
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
#define core_zero core_memset(&(x), 0, sizeof((x)))
#endif

#ifndef core_zerop
#define core_zerop core_memset((x), 0, sizeof(*(x)))
#endif
