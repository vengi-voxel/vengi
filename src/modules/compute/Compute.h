/**
 * @file
 * @defgroup Compute
 * @{
 *
 * The compute module contains wrappers around OpenCL.
 *
 * @see compute::Shader
 * @see ComputeShaderTool
 *
 * @}
 */
#pragma once

#include <string>
#include <string.h>
#include <vector>
#include "Types.h"
#include "Texture.h"
#include "cl/CLTypes.h"
#include "core/Vector.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace compute {

/**
 * @brief This will only return @c true if init() was called before.
 */
bool supported();
bool init();
void shutdown();

Id createBuffer(BufferFlag flags, size_t size = 0, void* data = nullptr);
bool updateBuffer(Id buffer, size_t size, const void* data, bool blockingWrite = true);
bool deleteBuffer(Id& buffer);
bool readBuffer(Id buffer, size_t size, void* data);

template<class T>
bool updateBufferFromType(std::true_type, Id buffer, T& data, bool blockingWrite = true) {
	return updateBuffer(buffer, core::vectorSize(data), const_cast<void*>((const void*)data.data()), blockingWrite);
}

template<class T>
bool updateBufferFromType(std::false_type, Id buffer, T& data, bool blockingWrite = true) {
	return updateBuffer(buffer, sizeof(T), const_cast<void*>(&data), blockingWrite);
}

template<class T>
bool updateBufferFromType(Id buffer, T& data, bool blockingWrite = true) {
	return updateBufferFromType(core::isVector<T> {}, buffer, data, blockingWrite);
}

template<class T>
Id createBufferFromType(std::true_type, BufferFlag flags, T& data) {
	if ((flags & BufferFlag::ReadOnly) != BufferFlag::None) {
		return createBuffer(flags, core::vectorCapacity(data), const_cast<void*>(reinterpret_cast<const void*>(data.data())));
	}
	return createBuffer(flags, core::vectorSize(data), const_cast<void*>(reinterpret_cast<const void*>(data.data())));
}

template<class T>
Id createBufferFromType(std::false_type, BufferFlag flags, T& data) {
	return createBuffer(flags, sizeof(T), const_cast<void*>(&data));
}

template<class T>
Id createBufferFromType(BufferFlag flags, T& data) {
	return createBufferFromType(core::isVector<T> {}, flags, data);
}

template<class T>
bool readBufferIntoVector(Id buffer, std::vector<T>& data) {
	return readBuffer(buffer, core::vectorCapacity(data), data.data());
}

Id createTexture(const Texture& texture, const uint8_t* data);
void deleteTexture(Id& id);

Id createSampler(const TextureConfig& config);
void deleteSampler(Id& id);

bool readTexture(compute::Texture& texture, void *data, const glm::uvec3& origin, const glm::uvec3& region, bool blocking = true);
Id createProgram(const std::string& source);
size_t requiredAlignment();
bool configureProgram(Id program);
bool deleteProgram(Id& program);

Id createKernel(Id program, const char *name);
bool deleteKernel(Id& kernel);
bool kernelArg(Id kernel, uint32_t index, const Texture& texture, int32_t samplerIndex = -1);
bool kernelArg(Id kernel, uint32_t index, size_t size, const void* data);
bool kernelRun(Id kernel, const glm::ivec3& globalWorkSize, int workDim, bool blocking = true);
bool finish();

template<class T>
inline bool kernelArg(Id kernel, uint32_t index, T& t) {
	return kernelArg(kernel, index, sizeof(T), (const void*)&t);
}

template<>
inline bool kernelArg(Id kernel, uint32_t index, const glm::vec3& t) {
	const glm::vec4 fourComponents(t, 0.0f);
	return kernelArg(kernel, index, sizeof(fourComponents), (const void*)&fourComponents);
}

template<>
inline bool kernelArg(Id kernel, uint32_t index, glm::vec3& t) {
	const glm::vec4 fourComponents(t, 0.0f);
	return kernelArg(kernel, index, sizeof(fourComponents), (const void*)&fourComponents);
}

/**
 * @brief Passing stuff like buffer or texture handles to the kernel
 */
template<>
inline bool kernelArg(Id kernel, uint32_t index, Id& t) {
	return kernelArg(kernel, index, sizeof(Id), (const void*)&t);
}

}
