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

bool VertexBuffer::addAttribute(const Attribute& attribute) {
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
		_vao = video::genVertexArray();
		video::bindVertexArray(_vao);
	}

	const int size = _attributes.size();
	for (int i = 0; i < size; i++) {
		const Attribute& a = _attributes[i];
		if (_targets[a.bufferIndex] == VertexBufferType::IndexBuffer) {
			continue;
		}
		video::bindBuffer(_targets[a.bufferIndex], _handles[a.bufferIndex]);
		video::configureAttribute(a);
	}
	for (unsigned int i = 0; i < _handleIdx; ++i) {
		if (_targets[i] != VertexBufferType::IndexBuffer) {
			continue;
		}
		if (_size[i] == 0u) {
			continue;
		}
		video::bindBuffer(_targets[i], _handles[i]);
	}
	video::bindVertexArray(InvalidId);
	for (uint32_t i = 0u; i < _handleIdx; ++i) {
		video::unbindBuffer(_targets[i]);
	}
	video::bindVertexArray(_vao);
	_dirtyAttributes = false;
	return true;
}

bool VertexBuffer::unbind() const {
	if (_vao == InvalidId) {
		return false;
	}
	if (video::boundVertexArray() == _vao) {
		video::bindVertexArray(InvalidId);
		return true;
	}
	return false;
}

bool VertexBuffer::update(int32_t idx, const void* data, size_t size) {
	if (!isValid(idx)) {
		return false;
	}

	const VertexBufferType type = _targets[idx];
	const Id id = _handles[idx];
	video::bindBuffer(type, id);
	if (_size[idx] >= size && _mode != VertexBufferMode::Static) {
		video::bufferSubData(type, 0, data, size);
	} else {
		video::bufferData(type, _mode, data, size);
	}
	if (video::boundVertexArray() != _vao) {
		video::unbindBuffer(type);
	}
	_size[idx] = size;

	return true;
}

int32_t VertexBuffer::create(const void* data, size_t size, VertexBufferType target) {
	// we already have a buffer
	if (_handleIdx >= (int)SDL_arraysize(_handles)) {
		return -1;
	}
	_targets[_handleIdx] = target;
	_handles[_handleIdx] = video::genBuffer();
	if (!isValid(_handleIdx)) {
		Log::error("Failed to create buffer (size: %i)", (int)size);
		return -1;
	}
	_size[_handleIdx] = size;
	if (data != nullptr) {
		video::bindBuffer(_targets[_handleIdx], _handles[_handleIdx]);
		video::bufferData(_targets[_handleIdx], _mode, data, size);
		video::unbindBuffer(_targets[_handleIdx]);
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
	core_assert_msg(_vao == InvalidId, "Vertex buffer was not properly shut down");
	shutdown();
}

void VertexBuffer::shutdown() {
	video::deleteVertexArray(_vao);
	video::deleteBuffers(_handleIdx, _handles);
	_handleIdx = 0;
	for (int i = 0; i < MAX_HANDLES; ++i) {
		_targets[0] = VertexBufferType::Max;
		_size[i] = 0u;
	}
	_mode = VertexBufferMode::Static;
	clearAttributes();
}

void VertexBuffer::clearAttributes() {
	_dirtyAttributes = false;
	_attributes.clear();
}

}
