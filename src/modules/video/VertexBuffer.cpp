/**
 * @file
 */

#include "VertexBuffer.h"
#include "core/Common.h"

namespace video {

VertexBuffer::VertexBuffer(const void* data, GLsizeiptr size, GLenum target) :
		_vao(0) {
	create(data, size, target);
}

VertexBuffer::VertexBuffer() :
		_vao(0) {
}

bool VertexBuffer::addAttribute(uint32_t attributeIndex, uint32_t bufferIndex, int size, GLenum type, bool normalized, int stride, intptr_t offset, uint8_t divisor) {
	_attributes.push_back(Attribute{attributeIndex, bufferIndex, size, type, stride, offset, normalized, divisor});
	return true;
}

bool VertexBuffer::bind() {
	if (!isValid(0)) {
		return false;
	}
	if (_vao != 0) {
		glBindVertexArray(_vao);
		return true;
	}

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);
	const int size = _attributes.size();
	for (int i = 0; i < size; i++) {
		const Attribute& a = _attributes[i];
		glBindBuffer(_targets[a.bufferIndex], _handles[a.bufferIndex]);
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, a.size, a.type, a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
		if (a.divisor > 0) {
			glVertexAttribDivisor(i, a.divisor);
		}
	}
	return true;
}

void VertexBuffer::unbind() {
	if (_vao != 0) {
		glBindVertexArray(0);
	} else {
		for (unsigned int i = 0; i < _handleIdx; ++i) {
			glBindBuffer(_targets[i], _handles[i]);
		}
	}
}

bool VertexBuffer::update(int index, const void* data, GLsizeiptr size) {
	if (!isValid(index)) {
		return false;
	}

	glBindBuffer(_targets[index], _handles[index]);
	glBufferData(_targets[index], size, data, GL_STATIC_DRAW);
	glBindBuffer(_targets[index], 0);

	return true;
}

int32_t VertexBuffer::create(const void* data, GLsizeiptr size, GLenum target) {
	// we already have a buffer
	if (_handleIdx >= (int)SDL_arraysize(_handles)) {
		return -1;
	}
	_targets[_handleIdx] = target;
	glGenBuffers(1, &_handles[_handleIdx]);
	if (!isValid(0)) {
		return -1;
	}
	glBindBuffer(target, _handles[_handleIdx]);
	_size[_handleIdx] = size;
	glBufferData(target, size, data, GL_STATIC_DRAW);
	glBindBuffer(target, 0);
	++_handleIdx;
	return _handleIdx - 1;
}

int32_t VertexBuffer::createFullscreenQuad() {
	static const glm::vec3 vecs[] = {
		glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f,  1.0f, 0.0f), glm::vec3( 1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f),
	};
	return create(vecs, sizeof(vecs));
}

int32_t VertexBuffer::createFullscreenTextureBuffer() {
	static const glm::vec2 vecs[] = {
		glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f),
		glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f),
	};
	return create(vecs, sizeof(vecs));
}

glm::ivec2 VertexBuffer::createFullscreenTexturedQuad() {
	glm::ivec2 indices;
	indices.x = createFullscreenQuad();
	indices.y = createFullscreenTextureBuffer();
	return indices;
}

VertexBuffer::~VertexBuffer() {
	shutdown();
}

void VertexBuffer::shutdown() {
	if (_vao != 0) {
		glDeleteVertexArrays(1, &_vao);
		_vao = 0;
	}
	if (_handleIdx > 0) {
		glDeleteBuffers(_handleIdx, _handles);
		_handleIdx = 0;
	}
	_attributes.clear();
}

}
