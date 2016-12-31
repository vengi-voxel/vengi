/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include "core/GLM.h"
#include <vector>

namespace video {

enum class VertexBufferMode : GLenum {
	Static = GL_STATIC_DRAW,
	Dynamic = GL_DYNAMIC_DRAW
};

enum class VertexBufferType : GLenum {
#ifdef GL_DRAW_INDIRECT_BUFFER
	DrawIndirectBuffer = GL_DRAW_INDIRECT_BUFFER,
#endif
	ArrayBuffer = GL_ARRAY_BUFFER,
	IndexBuffer = GL_ELEMENT_ARRAY_BUFFER,
	UniformBuffer = GL_UNIFORM_BUFFER
};

class VertexBuffer {
public:
	struct Attribute {
		/** shader attribute index */
		uint32_t index = 0u;
		/** The internal buffer index that was returned by @c create() */
		uint32_t bufferIndex = 0u;
		/** The size behind your attribute (not sizeof but lengthof). */
		int size = 0;
		/** the amount of bytes between each attribute instance */
		int stride = 0;
		/** the offset of the buffer to start reading from */
		intptr_t offset = 0;
		/** The data type behind your attribute - also see @c typeIsInt */
		GLenum type = GL_FLOAT;
		/**
		 * The rate by which the attribute advances during instanced rendering. It basically means the number of
		 * times the entire set of vertices is rendered before the attribute is updated from the buffer. By default,
		 * the divisor is zero. This causes regular vertex attributes to be updated from vertex to vertex. If the divisor
		 * is 10 it means that the first 10 instances will use the first piece of data from the buffer, the next 10 instances
		 * will use the second, etc. We want to have a dedicated WVP matrix for each instance so we use a divisor of 1.
		 */
		uint8_t divisor = 0;
		bool normalized = false;
		/** use glVertexAttribPointer or glVertexAttribIPointer for uploading */
		bool typeIsInt = false;
	};
private:
	static constexpr int MAX_HANDLES = 6;
	GLuint _size[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u};
	GLuint _handles[MAX_HANDLES] = {GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE};
	GLenum _targets[MAX_HANDLES] = {GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE};
	GLuint _handleIdx = 0;

	std::vector<Attribute> _attributes;
	mutable GLuint _vao = 0u;
	VertexBufferMode _mode = VertexBufferMode::Static;
public:
	VertexBuffer(const void* data, GLsizeiptr size, VertexBufferType target = VertexBufferType::ArrayBuffer);
	VertexBuffer();
	~VertexBuffer();

	void setMode(VertexBufferMode mode);
	void shutdown();

	bool addAttribute(const Attribute& attribute);

	template<class T>
	inline bool update(int32_t idx, const std::vector<T>& data) {
		const T* dataPtr = nullptr;
		if (!data.empty()) {
			dataPtr = &data.front();
		}
		return update(idx, dataPtr, core::vectorSize(data));
	}

	bool update(int32_t idx, const void* data, GLsizeiptr size);

	template<class T>
	inline int32_t create(const std::vector<T>& data, VertexBufferType target = VertexBufferType::ArrayBuffer) {
		return create(&data.front(), core::vectorSize(data), target);
	}

	int32_t create(const void* data = nullptr, GLsizeiptr size = 0, VertexBufferType target = VertexBufferType::ArrayBuffer);
	/**
	 * @brief Fullscreen buffer with normalized device coordinates with 3 float components
	 */
	int32_t createFullscreenQuad();
	/**
	 * @brief Full texture coordinate buffer with 2 float components
	 */
	int32_t createFullscreenTextureBuffer();
	int32_t createFullscreenTextureBufferYFlipped();
	glm::ivec2 createFullscreenTexturedQuad(bool yFlipped = false);
	/**
	 * @brief Screen coordinate buffer with 2 float components for vertices and 2 float components for texcoords
	 * @return Two vertex buffers, the first one contains the vertices, the second contains the texcoords
	 */
	glm::ivec2 createTexturedQuad(const glm::ivec2& xy, const glm::ivec2& dimension);
	int32_t createWhiteColorForQuad();
	bool bind() const;
	void unbind() const;
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

inline void VertexBuffer::setMode(VertexBufferMode mode) {
	_mode = mode;
}

}
