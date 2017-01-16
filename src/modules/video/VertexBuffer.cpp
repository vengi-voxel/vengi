/**
 * @file
 */

#include "VertexBuffer.h"
#include "core/Common.h"
#include <vector>

namespace video {

VertexBuffer::VertexBuffer(const void* data, size_t size, VertexBufferType target) {
	create(data, size, target);
}

VertexBuffer::VertexBuffer() {
}

bool VertexBuffer::addAttribute(const VertexBuffer::Attribute& attribute) {
	if (attribute.bufferIndex < 0) {
		return false;
	}
	if (attribute.index < 0) {
		return false;
	}
	if (attribute.size <= 0) {
		return false;
	}
	_attributes.push_back(attribute);
	_dirtyAttributes = true;
	return true;
}

bool VertexBuffer::bind() const {
	if (!isValid(0)) {
		return false;
	}
	if (_vao != InvalidId) {
		video::bindVertexArray(_vao);
		if (!_dirtyAttributes) {
			return true;
		}
	} else {
		glGenVertexArrays(1, &_vao);
		video::bindVertexArray(_vao);
	}

	const int size = _attributes.size();
	for (int i = 0; i < size; i++) {
		const Attribute& a = _attributes[i];
		if (_targets[a.bufferIndex] == GL_ELEMENT_ARRAY_BUFFER) {
			continue;
		}
		glBindBuffer(_targets[a.bufferIndex], _handles[a.bufferIndex]);
		glEnableVertexAttribArray(a.index);
		if (a.typeIsInt) {
			glVertexAttribIPointer(a.index, a.size, a.type, a.stride, GL_OFFSET_CAST(a.offset));
		} else {
			glVertexAttribPointer(a.index, a.size, a.type, a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
		}
		if (a.divisor > 0) {
			glVertexAttribDivisor(a.index, a.divisor);
		}
	}
	for (unsigned int i = 0; i < _handleIdx; ++i) {
		if (_targets[i] != GL_ELEMENT_ARRAY_BUFFER) {
			continue;
		}
		if (_size[i] == 0u) {
			continue;
		}
		glBindBuffer(_targets[i], _handles[i]);
	}
	_dirtyAttributes = false;
	GL_checkError();
	return true;
}

void VertexBuffer::unbind() const {
	if (_vao != InvalidId) {
		glBindVertexArray(0);
	} else {
		for (unsigned int i = 0; i < _handleIdx; ++i) {
			glBindBuffer(_targets[i], 0);
		}
	}
}

bool VertexBuffer::update(int32_t idx, const void* data, size_t size) {
	if (!isValid(idx)) {
		return false;
	}

	glBindBuffer(_targets[idx], _handles[idx]);
	if (_size[idx] >= size && _mode == VertexBufferMode::Dynamic) {
		glBufferSubData(_targets[idx], 0, (GLsizeiptr)size, data);
	} else {
		glBufferData(_targets[idx], (GLsizeiptr)size, data, std::enum_value(_mode));
	}
	glBindBuffer(_targets[idx], 0);
	_size[idx] = size;
	GL_checkError();

	return true;
}

int32_t VertexBuffer::create(const void* data, size_t size, VertexBufferType target) {
	// we already have a buffer
	if (_handleIdx >= (int)SDL_arraysize(_handles)) {
		return -1;
	}
	_targets[_handleIdx] = std::enum_value(target);
	glGenBuffers(1, &_handles[_handleIdx]);
	if (!isValid(0)) {
		return -1;
	}
	_size[_handleIdx] = size;
	if (data != nullptr) {
		glBindBuffer(_targets[_handleIdx], _handles[_handleIdx]);
		glBufferData(_targets[_handleIdx], (GLsizeiptr)size, data, std::enum_value(_mode));
		glBindBuffer(_targets[_handleIdx], 0);
	}
	++_handleIdx;
	return _handleIdx - 1;
}

int32_t VertexBuffer::createFullscreenQuad() {
	// counter clock wise winding
	//
	// -1/1    1/1
	// -------------
	// |     |     |
	// |     |0/0  |
	// -------------
	// |     |     |
	// |     |     |
	// -------------
	// -1/-1    1/-1
	//
	static const glm::vec3 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3( 1.0f,  1.0f, 0.0f),
		// left bottom, right top, left top
		glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(-1.0f,  1.0f, 0.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t VertexBuffer::createFullscreenTextureBuffer() {
	// counter clock wise winding
	//
	// 0/0    1/0
	// ----------
	// |        |
	// |        |
	// |        |
	// ----------
	// 0/1    1/1
	//
	static const glm::vec2 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f),
		// left bottom, right top, left top
		glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t VertexBuffer::createFullscreenTextureBufferYFlipped() {
	// counter clock wise winding
	//
	// 0/1    1/1
	// ----------
	// |        |
	// |        |
	// |        |
	// ----------
	// 0/0    1/0
	//
	static const glm::vec2 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f),
		// left bottom, right top, left top
		glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t VertexBuffer::createWhiteColorForQuad() {
	static const glm::vec4 color[] = {
		glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
	};
	return create(color, sizeof(color));
}

glm::ivec2 VertexBuffer::createTexturedQuad(const glm::ivec2& xy, const glm::ivec2& dimension) {
	// counter clock wise winding
	//
	// -1/1    1/1
	// -------------
	// |     |     |
	// |     |0/0  |
	// -------------
	// |     |     |
	// |     |     |
	// -------------
	// -1/-1    1/-1
	//
	const glm::vec2 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec2(xy.x, xy.y + dimension.y), glm::vec2(xy.x + dimension.x, xy.y + dimension.y), glm::vec2(xy.x + dimension.x, xy.y),
		// left bottom, right top, left top
		glm::vec2(xy.x, xy.y + dimension.y), glm::vec2(xy.x + dimension.x, xy.y), glm::vec2(xy.x, xy.y)
	};
	glm::ivec2 indices;
	indices.x = create(vecs, sizeof(vecs));
	indices.y = createFullscreenTextureBuffer();
	return indices;
}

glm::ivec2 VertexBuffer::createFullscreenTexturedQuad(bool yFlipped) {
	glm::ivec2 indices;
	indices.x = createFullscreenQuad();
	if (yFlipped) {
		indices.y = createFullscreenTextureBufferYFlipped();
	} else {
		indices.y = createFullscreenTextureBuffer();
	}
	return indices;
}

VertexBuffer::~VertexBuffer() {
	core_assert_msg(_vao == 0u, "Vertex buffer was not properly shut down");
	shutdown();
}

void VertexBuffer::shutdown() {
	if (_vao != InvalidId) {
		glDeleteVertexArrays(1, &_vao);
		_vao = InvalidId;
	}
	if (_handleIdx > 0) {
		glDeleteBuffers(_handleIdx, _handles);
		_handleIdx = 0;
	}
	clearAttributes();
}

void VertexBuffer::clearAttributes() {
	_attributes.clear();
}

}
