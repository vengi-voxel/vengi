/**
 * @file
 *
 * @ingroup Compute
 */
#include "compute/cuda/CUDACompute.h"
#include "video/Buffer.h"
#include "video/Texture.h"

namespace computevideo {

bool init() {
	return false;
}

void shutdown() {
}

compute::Id createBuffer(compute::BufferFlag flags, video::Buffer& buffer, int idx) {
	return compute::InvalidId;
}

compute::Id createTexture(compute::BufferFlag flags, video::Texture& texture) {
	return compute::InvalidId;
}

bool enqueueAcquire(compute::Id id) {
	return false;
}

bool enqueueRelease(compute::Id id) {
	return false;
}

}
