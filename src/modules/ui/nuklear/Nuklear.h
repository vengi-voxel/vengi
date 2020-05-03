/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Common.h"
#include "core/StandardLib.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_ASSERT core_assert
#define STBTT_malloc(x,u)  core_malloc(x)
#define STBTT_free(x,u)    core_free(x)
#define NK_SQRT

/**
 * @addtogroup UI
 * @{
 */
#include "private/nuklear.h"
/**
 * @}
 */
