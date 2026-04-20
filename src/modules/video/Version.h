/**
 * @file
 */

#pragma once

#include "engine-config.h"

#if USE_GL_RENDERER
#include "gl/GLVersion.h"
#elif USE_VK_RENDERER
// Vulkan version info is queried at runtime via vkEnumerateInstanceVersion
#else
#error "No video backend selected"
#endif
