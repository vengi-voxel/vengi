/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include "core/GLM.h"
#include <vector>

namespace video {

class VertexBuffer {
private:
	static constexpr int MAX_HANDLES = 6;
	GLuint _size[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u};
	GLuint _handles[MAX_HANDLES] = {GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE};
	GLenum _targets[MAX_HANDLES] = {GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE};
	GLuint _handleIdx = 0;
	struct Attribute {
		uint32_t index;
		uint32_t bufferIndex;
		int size;
		GLenum type;
		int stride;
		intptr_t offset;
		bool normalized = false;
		uint8_t divisor = 0; // for instanced rendering
	};
	std::vector<Attribute> _attributes;
	GLuint _vao;
public:
	VertexBuffer(const void* data, GLsizeiptr size, GLenum target = GL_ARRAY_BUFFER);
	VertexBuffer();
	~VertexBuffer();

	void shutdown();

	/**
	 * @param[in] attributeIndex The index in the shader
	 * @param[in] bufferIndex The internal buffer index that was returned by @c create()
	 * @param[in] size The size behind your attribute (not sizeof but lengthof).
	 * @param[in] type The data type behind your attribute
	 * @param[in] normalized
	 * @param[in] offset The offset of the member inside your buffer struct
	 * @param[in] divisor The rate by which the attribute advances during instanced rendering. It basically means the number of
	 * times the entire set of vertices is rendered before the attribute is updated from the buffer. By default,
	 * the divisor is zero. This causes regular vertex attributes to be updated from vertex to vertex. If the divisor
	 * is 10 it means that the first 10 instances will use the first piece of data from the buffer, the next 10 instances
	 * will use the second, etc. We want to have a dedicated WVP matrix for each instance so we use a divisor of 1.
	 */
	bool addAttribute(uint32_t attributeIndex, uint32_t bufferIndex, int size, GLenum type = GL_FLOAT, bool normalized = false, int stride = 0, intptr_t offset = 0, uint8_t divisor = 0);

	bool update(int32_t idx, const void* data, GLsizeiptr size);
	int32_t create(const void* data, GLsizeiptr size, GLenum target = GL_ARRAY_BUFFER);
	int32_t createFullscreenQuad();
	int32_t createFullscreenTextureBuffer();
	glm::ivec2 createFullscreenTexturedQuad();
	bool bind();
	void unbind();
	GLuint size(int32_t idx) const;
	GLuint elements(int32_t idx, int components = 3, size_t componentSize = sizeof(float)) const;
	bool isValid(int32_t idx) const;
	GLuint handle(int32_t idx) const;
};

inline bool VertexBuffer::isValid(int32_t idx) const {
	if (idx < 0) {
		return false;
	}
	if (idx >= (int)SDL_arraysize(_handles)) {
		return false;
	}
	return _handles[idx] != GL_INVALID_VALUE && _handles[idx] > 0;
}

inline GLuint VertexBuffer::size(int32_t idx) const {
	core_assert(idx >= 0 && idx < (int)SDL_arraysize(_size));
	return _size[idx];
}

inline GLuint VertexBuffer::elements(int32_t idx, int components, size_t componentSize) const {
	return size(idx) / (components * componentSize);
}

inline GLuint VertexBuffer::handle(int32_t idx) const {
	core_assert(idx >= 0 && idx < (int)SDL_arraysize(_handles));
	return _handles[idx];
}

}
