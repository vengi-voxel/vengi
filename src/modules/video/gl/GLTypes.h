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
using IdPtr = intptr_t;
constexpr Id InvalidId = (Id)0;
constexpr IdPtr InvalidIdPtr = (IdPtr)0;
typedef void* RendererContext;

}
