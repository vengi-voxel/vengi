/**
 * @file
 */

#include "core/StandardLib.h"
#include "core/collection/BufferView.h"

#if __GNUC__
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough" // stb_image.h
#endif

#define STBI_ASSERT core_assert
#define STBI_MALLOC core_malloc
#define STBI_REALLOC core_realloc
#define STBI_FREE core_free
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STBIW_ASSERT core_assert
#define STBIW_MALLOC core_malloc
#define STBIW_REALLOC core_realloc
#define STBIW_FREE core_free
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STBIR_ASSERT core_assert
#define STBIR_MALLOC(size, user_data) ((void)(user_data), core_malloc(size))
#define STBIR_FREE(ptr, user_data) ((void)(user_data), core_free(ptr))
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#undef STB_IMAGE_RESIZE_IMPLEMENTATION
