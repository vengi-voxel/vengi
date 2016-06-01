#pragma once

#include "GLFunc.h"
#include "core/Common.h"

namespace video {

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
