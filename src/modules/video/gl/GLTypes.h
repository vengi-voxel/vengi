/**
 * @file
 */

#pragma once

#include <cstdint>

namespace video {

/**
 * @brief OpenGL resource handle
 */
using Id = uint32_t;
constexpr Id InvalidId = (Id)0;
typedef void* RendererContext;

}
