/**
 * @file
 */

#ifndef TB_CONFIG_H
#define TB_CONFIG_H

#include <SDL_platform.h>

#ifdef DEBUG
#define TB_RUNTIME_DEBUG_INFO
#endif
#define TB_FONT_RENDERER_TBBF
#define TB_IMAGE_LOADER_STB
#define TB_RENDERER_BATCHER
#define TB_GLYPH_CACHE_WIDTH 512
#define TB_GLYPH_CACHE_HEIGHT 512
#define TB_IMAGE
#define TB_CLIPBOARD_SDL
#define TB_SYSTEM_SDL
#define TB_FILE_SDL

#ifdef __WINDOWS__
#define TB_TARGET_WINDOWS
#endif

#ifdef __LINUX__
#define TB_TARGET_LINUX
#endif

#ifdef __MACOSX__
#define TB_TARGET_MACOSX
#endif

#endif
