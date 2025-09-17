/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace video {

/**
 * @brief Vulkan resource handle
 */
using Id = uint32_t; // TODO: VULKAN: check me
using IdPtr = intptr_t;
constexpr Id InvalidId = (Id)0;
constexpr IdPtr InvalidIdPtr = (IdPtr)0;
typedef void *RendererContext;

} // namespace video
