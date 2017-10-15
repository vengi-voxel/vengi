/**
 * @file
 */

#ifndef TB_CONFIG_H
#define TB_CONFIG_H

#ifdef DEBUG
#define TB_RUNTIME_DEBUG_INFO
#endif
#define TB_FONT_RENDERER_TBBF
#define TB_FONT_RENDERER_STB
#define TB_IMAGE_LOADER_STB
#define TB_RENDERER_BATCHER
#define TB_GLYPH_CACHE_WIDTH 512
#define TB_GLYPH_CACHE_HEIGHT 512
#define TB_IMAGE
#define TB_CLIPBOARD_SDL
#define TB_SYSTEM_SDL
#define TB_FILE_SDL

#if defined(__linux) || defined(__linux__)
#define TB_TARGET_LINUX
#elif MACOSX
#define TB_TARGET_MACOSX
#elif defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#define TB_TARGET_WINDOWS
#endif

#endif
