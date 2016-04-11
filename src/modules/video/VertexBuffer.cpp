#include "VertexBuffer.h"
#include "core/Common.h"

namespace video {

VertexBuffer::VertexBuffer(const void* data, GLsizeiptr size, GLenum target) :
		_handle(GL_INVALID_VALUE), _target(target), _vao(0) {
	create(data, size, target);
}

VertexBuffer::VertexBuffer() :
		_handle(GL_INVALID_VALUE), _target(GL_INVALID_VALUE), _vao(0) {
}

bool VertexBuffer::addAttribute(uint32_t index, int size, GLenum type, bool normalized, int stride, intptr_t offset) {
	// if we already have a buffer, we don't accept new attributes
	if (isValid())
		return false;
	_attributes.push_back(Attribute{index, size, type, normalized, stride, offset});
	return true;
}

bool VertexBuffer::bind() {
	if (!isValid())
		return false;
	if (_vao != 0) {
		glBindVertexArray(_vao);
		return true;
	}

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);
	glBindBuffer(_target, _handle);
	const int size = _attributes.size();
	for (int i = 0; i < size; i++) {
		const Attribute& a = _attributes[i];
		glVertexAttribPointer(i, a.size, a.type, a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
		glEnableVertexAttribArray(i);
	}
	return true;
}

void VertexBuffer::unbind() {
	if (_vao != 0)
		glBindVertexArray(0);
	else
		glBindBuffer(_target, 0);
}

bool VertexBuffer::create(const void* data, GLsizeiptr size, GLenum target) {
	// we already have a buffer
	if (isValid()) {
		core_assert(_target == target);
		glBindBuffer(target, _handle);
		glBufferData(target, size, data, GL_STATIC_DRAW);
		glBindBuffer(target, 0);
		return true;
	}
	_target = target;
	glGenBuffers(1, &_handle);
	if (!isValid())
		return false;
	glBindBuffer(target, _handle);
	glBufferData(target, size, data, GL_STATIC_DRAW);
	glBindBuffer(target, 0);
	return true;
}

VertexBuffer::~VertexBuffer() {
	if (!isValid())
		return;
	if (_vao != 0)
		glDeleteVertexArrays(1, &_vao);
	glDeleteBuffers(1, &_handle);
}

}
