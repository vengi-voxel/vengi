/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include "core/NonCopyable.h"

namespace video {

/**
 * @brief A Buffer Object that is used to store data accessible by shaders.
 *
 * Shader Storage Buffer Objects (SSBOs) are similar to Uniform Buffer Objects
 * but with several important differences:
 *
 * - SSBOs can be much larger (typically limited only by GPU memory)
 * - Shaders can write to SSBOs (not just read)
 * - SSBOs support variable-length arrays as the last member
 * - SSBOs use std430 layout by default (more efficient packing than std140)
 *
 * SSBOs are commonly used for:
 * - Compute shader input/output
 * - Large data sets that don't fit in uniform buffers
 * - GPU-side data structures that need read/write access
 *
 * @ingroup Video
 */
class ShaderStorageBuffer : public core::NonCopyable {
private:
	Id _handle = InvalidId;
	size_t _size = 0;

public:
	~ShaderStorageBuffer();

	void shutdown();

	Id handle() const;

	/**
	 * @brief Create the buffer with initial data
	 * @param data Pointer to initial data (can be nullptr to just allocate)
	 * @param size Size in bytes of the buffer
	 * @return true on success
	 */
	bool create(const void *data, size_t size);

	/**
	 * @brief Update the entire buffer with new data
	 * @param data Pointer to the data to upload
	 * @param size Size in bytes of the data
	 * @return true on success
	 */
	bool update(const void *data, size_t size);

	/**
	 * @brief Update a portion of the buffer
	 * @param offset Byte offset into the buffer
	 * @param data Pointer to the data to upload
	 * @param size Size in bytes of the data
	 * @return true on success
	 */
	bool update(size_t offset, const void *data, size_t size);

	/**
	 * @brief Get the size of the buffer in bytes
	 */
	size_t size() const;

	/**
	 * @brief Bind the buffer to a shader storage buffer binding point
	 * @param index The binding point index (corresponds to layout(binding = N) in GLSL)
	 * @return true on success
	 */
	bool bind(uint32_t index = 0u) const;

	/**
	 * @brief Map the buffer for CPU access
	 * @param mode Access mode (Read, Write, or ReadWrite)
	 * @return Pointer to the mapped memory, or nullptr on failure
	 */
	void *map(AccessMode mode = AccessMode::ReadWrite);

	/**
	 * @brief Unmap a previously mapped buffer
	 */
	void unmap();
};

inline size_t ShaderStorageBuffer::size() const {
	return _size;
}

inline Id ShaderStorageBuffer::handle() const {
	return _handle;
}

} // namespace video