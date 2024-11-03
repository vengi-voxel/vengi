/**
 * @file
 */

#include <SDL_endian.h>

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define CORE_LITTLE_ENDIAN 1
#else
#define CORE_LITTLE_ENDIAN 0
#endif

#define core_swap32le SDL_SwapLE32
#define core_swap32be SDL_SwapBE32
#define core_swap16le SDL_SwapLE16
#define core_swap16be SDL_SwapBE16
#define core_swap64le SDL_SwapLE64
#define core_swap64be SDL_SwapBE64
