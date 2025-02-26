/**
 * @file
 */

#include <SDL_endian.h>
#include <SDL_version.h>

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define CORE_LITTLE_ENDIAN 1
#else
#define CORE_LITTLE_ENDIAN 0
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define core_swap32le SDL_Swap32LE
#define core_swap32be SDL_Swap32BE
#define core_swap16le SDL_Swap16LE
#define core_swap16be SDL_Swap16BE
#define core_swap64le SDL_Swap64LE
#define core_swap64be SDL_Swap64BE
#else
#define core_swap32le SDL_SwapLE32
#define core_swap32be SDL_SwapBE32
#define core_swap16le SDL_SwapLE16
#define core_swap16be SDL_SwapBE16
#define core_swap64le SDL_SwapLE64
#define core_swap64be SDL_SwapBE64
#endif
