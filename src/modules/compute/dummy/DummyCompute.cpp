/**
 * @file
 *
 * @ingroup Compute
 */
#include "compute/Compute.h"

namespace compute {

size_t requiredAlignment() {
	return 0;
}

bool configureProgram(Id program) {
	return false;
}

bool deleteProgram(Id& program) {
	if (program == InvalidId) {
		return true;
	}
	return false;
}

Id createBuffer(BufferFlag flags, size_t size, void* data) {
	return InvalidId;
}

bool deleteBuffer(Id& buffer) {
	if (buffer == InvalidId) {
		return true;
	}
	return false;
}

bool updateBuffer(Id buffer, size_t size, const void* data, bool blockingWrite) {
	return false;
}

bool readBuffer(Id buffer, size_t size, void* data) {
	return false;
}

Id createTexture(const Texture& texture, const uint8_t* data) {
	return InvalidId;
}

void deleteTexture(Id& id) {
}

Id createSampler(const TextureConfig& config) {
	return InvalidId;
}

void deleteSampler(Id& id) {
}

bool readTexture(compute::Texture& texture, void *data, const glm::ivec3& origin, const glm::ivec3& region, bool blocking) {
	return false;
}

bool copyBufferToImage(compute::Id buffer, compute::Id image, size_t bufferOffset, const glm::ivec3& origin, const glm::ivec3& region) {
	return false;
}

Id createProgram(const std::string& source) {
	return InvalidId;
}

bool deleteKernel(Id& kernel) {
	return false;
}

bool kernelArg(Id kernel, uint32_t index, const Texture& texture, int32_t samplerIndex) {
	return false;
}


bool kernelArg(Id kernel, uint32_t index, size_t size, const void* data) {
	return false;
}

bool kernelRun(Id kernel, const glm::ivec3& workSize, int workDim, bool blocking) {
	return false;
}

Id createKernel(Id program, const char *name) {
	return InvalidId;
}

bool finish() {
	return false;
}

bool supported() {
	return false;
}

bool init() {
	return false;
}

void shutdown() {
}

bool hasFeature(Feature f) {
	return false;
}

}
