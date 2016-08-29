#pragma once

#include "GLFunc.h"
#include "core/Common.h"

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
public:
	UniformBuffer();
	~UniformBuffer();

	void shutdown();

	void create(GLsizeiptr size, const void *data);

	/**
	 * @param[in] index The index of the uniform block to bind the buffer to
	 */
	void bind(GLuint index);
};

}
