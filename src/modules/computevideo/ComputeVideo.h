/**
 * @file
 * @defgroup Compute
 * @{
 *
 * The computevideo module contains wrappers around the OpenGL integration with OpenCL.
 *
 * @}
 */
#pragma once

#include "compute/Types.h"

#define COMPUTEVIDEO

namespace video {
class Buffer;
class Texture;
}

namespace computevideo {

/**
 * @brief This must be initialized before the compute::init() method was called
 */
bool init();
void shutdown();

compute::Id createTexture(compute::BufferFlag flags, video::Texture& texture);
compute::Id createBuffer(compute::BufferFlag flags, video::Buffer& buffer, int idx = 0);
bool enqueueAcquire(compute::Id id);
bool enqueueRelease(compute::Id id);

}
