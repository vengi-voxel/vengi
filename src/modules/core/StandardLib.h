/**
 * @file
 */

#pragma once

#include <string.h>

#ifndef SDLCALL
#if defined(_WIN32) && !defined(__GNUC__)
#define SDLCALL __cdecl
#else
#define SDLCALL
#endif
#endif

extern "C" void *SDLCALL SDL_malloc(size_t size);
extern "C" void *SDLCALL SDL_realloc(void *mem, size_t size);
extern "C" void SDLCALL SDL_free(void *mem);
extern "C" void *SDLCALL SDL_memset(void *dst, int c, size_t len);
extern "C" void *SDLCALL SDL_memcpy(void *dst, const void *src, size_t len);
extern "C" int SDLCALL SDL_memcmp(const void *s1, const void *s2, size_t len);
extern "C" char *SDLCALL SDL_strdup(const char *str);

#ifdef _MSC_VER
#define core_strcasecmp _stricmp
#else
#define core_strcasecmp strcasecmp
#endif

#define DEBUG_MALLOC 0
#ifndef core_malloc
# if DEBUG_MALLOC
#include <stdio.h>

static inline void* debug_core_malloc(size_t size, const char* file, int line) {
	void* ptr = SDL_malloc(size);
	printf("malloc %p of size %zu at %s:%d\n", ptr, size, file, line);
	return ptr;
}
#  define core_malloc(size) debug_core_malloc(size, __FILE__, __LINE__)
# else
#  define core_malloc SDL_malloc
# endif
#endif

#ifndef core_realloc
#define core_realloc SDL_realloc
#endif

#ifndef core_free
#define core_free SDL_free
#endif

#ifndef core_strdup
#define core_strdup SDL_strdup
#endif

#ifndef core_memset
#define core_memset SDL_memset
#endif

#ifndef core_memset4
#define core_memset4 SDL_memset4
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
