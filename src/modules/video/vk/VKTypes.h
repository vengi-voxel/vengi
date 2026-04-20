/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace video {

/**
 * @brief Vulkan resource handle.
 *
 * The video backend uses opaque uint32_t IDs for all GPU resources (buffers,
 * textures, programs, framebuffers, ...). The VK backend maps these IDs to
 * actual Vulkan handles via internal registries inside VkRenderer.cpp.
 * ID 0 (InvalidId) is always reserved as the null/invalid sentinel.
 */
using Id = uint32_t;
using IdPtr = intptr_t;
constexpr Id InvalidId = (Id)0;
constexpr IdPtr InvalidIdPtr = (IdPtr)0;
typedef void *RendererContext;

} // namespace video
