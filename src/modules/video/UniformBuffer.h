#pragma once

#include "Renderer.h"

namespace video {

/**
 * @brief A Buffer Object that is used to store uniform data for a shader program.
 *
 * Uniform buffers have several uses.
 *
 * Switching between uniform buffer bindings is typically faster than switching dozens of uniforms in a program.
 * Therefore, uniform buffers can be used to quickly change between different sets of uniform data for different
 * objects that share the same program.
 *
 * Also, uniform buffer objects can typically store more data than non-buffered uniforms. So they can be used to
 * store and access larger blocks of data than unbuffered uniform values.
 *
 * Lastly, they can be used to share information between different programs. So modifying a single buffer can
 * effectively allow uniforms in multiple programs to be updated.
 */
class UniformBuffer {
private:
	Id _handle = InvalidId;

public:
	~UniformBuffer();

	void shutdown();

	Id handle() const;
	bool create(const void *data, size_t size);
	bool update(const void *data, size_t size);

	/**
	 * @param[in] index The index of the uniform block to bind the buffer to
	 */
	bool bind(uint32_t index = 0u) const;
};

inline Id UniformBuffer::handle() const {
	return _handle;
}

}
