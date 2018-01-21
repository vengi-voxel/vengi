/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace video {

/**
 * @brief OpenGL resource handle
 */
using Id = uint32_t;
constexpr Id InvalidId = (Id)0;
typedef void* RendererContext;

}
