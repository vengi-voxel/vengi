/**
 * @file
 */

 #include "core/StandardLib.h"
 #include "core/collection/BufferView.h"

 #define STBI_ASSERT core_assert
 #define STBI_MALLOC core_malloc
 #define STBI_REALLOC core_realloc
 #define STBI_FREE core_free
 #define STBI_NO_FAILURE_STRINGS

 #define STBIW_ASSERT core_assert
 #define STBIW_MALLOC core_malloc
 #define STBIW_REALLOC core_realloc
 #define STBIW_FREE core_free

 #if __GNUC__
 #pragma GCC diagnostic ignored "-Wimplicit-fallthrough" // stb_image.h
 #endif

 #define STBI_NO_STDIO
 #define STB_IMAGE_IMPLEMENTATION
 #include <stb_image.h>

 #define STBI_WRITE_NO_STDIO
 #define STB_IMAGE_WRITE_IMPLEMENTATION
 #include <stb_image_write.h>

 #undef STB_IMAGE_IMPLEMENTATION
 #undef STB_IMAGE_WRITE_IMPLEMENTATION
