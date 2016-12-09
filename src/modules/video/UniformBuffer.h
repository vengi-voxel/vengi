#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include "BufferLockMode.h"

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
	GLuint _handle;
	GLsizeiptr _size = 0;

public:
	UniformBuffer();
	~UniformBuffer();

	void shutdown();

	GLuint handle() const;
	void create(GLsizeiptr size, const void *data);

	void* lock(BufferLockMode mode);
	void unlock();

	/**
	 * @param[in] index The index of the uniform block to bind the buffer to
	 */
	void bind(GLuint index = 0) const;
};

inline GLuint UniformBuffer::handle() const {
	return _handle;
}

}
