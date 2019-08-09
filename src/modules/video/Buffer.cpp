/**
 * @file
 */

#include "Buffer.h"
#include "core/Common.h"
#include <vector>

namespace video {

Buffer::Buffer(const void* data, size_t size, BufferType target) {
	create(data, size, target);
}

Buffer::Buffer() {
}

size_t Buffer::bufferSize(int32_t idx) const {
	if (!isValid(idx)) {
		return 0u;
	}

	const BufferType type = _targets[idx];
	core_assert(video::boundBuffer(type) == _handles[idx]);
	return video::bufferSize(type);
}

void* Buffer::mapData(int32_t idx, video::AccessMode mode) const {
	if (!isValid(idx)) {
		return nullptr;
	}
	bind();
	const BufferType type = _targets[idx];
	return video::mapBuffer(_handles[idx], type, mode);
}

void Buffer::unmapData(int32_t idx) const {
	if (!isValid(idx)) {
		return;
	}
	const BufferType type = _targets[idx];
	video::unmapBuffer(_handles[idx], type);
}

bool Buffer::addAttribute(const Attribute& attribute) {
	if (attribute.bufferIndex < 0) {
		Log::debug("No buffer index is set");
		return false;
	}
	if (attribute.location < 0) {
		Log::debug("No attribute location is set");
		return false;
	}
	if (attribute.size <= 0) {
		Log::debug("No attribute size is set");
		return false;
	}
	if (!video::checkLimit(_attributes.size(), video::Limit::MaxVertexAttribs)) {
		Log::error("The max vertex attributes are exceeded");
		return false;
	}
	_attributes.push_back(attribute);
	_dirtyAttributes = true;
	return true;
}

bool Buffer::bind() const {
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
		if (_targets[a.bufferIndex] != BufferType::ArrayBuffer) {
			continue;
		}
		video::bindBuffer(_targets[a.bufferIndex], _handles[a.bufferIndex]);
		video::configureAttribute(a);
	}
	for (unsigned int i = 0; i < _handleIdx; ++i) {
		if (_targets[i] != BufferType::IndexBuffer) {
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

bool Buffer::unbind() const {
	if (_vao == InvalidId) {
		return false;
	}
	if (video::boundVertexArray() == _vao) {
		video::bindVertexArray(InvalidId);
		return true;
	}
	return false;
}

bool Buffer::update(int32_t idx, const void* data, size_t size) {
	if (!isValid(idx)) {
		return false;
	}

	core_assert(video::boundVertexArray() == InvalidId);
	const size_t oldSize = _size[idx];
	const size_t newSize = align(size, _targets[idx]);
#if VIDEO_BUFFER_HASH_COMPARE
	if (oldSize == newSize) {
		uint32_t newHash = core::hash(data, size);
		if (newHash == _hash[idx]) {
			return true;
		}
		_hash[idx] = newHash;
	} else {
		_hash[idx] = core::hash(data, size);
	}
#endif
	_size[idx] = newSize;
	core_assert_16byte_aligned(data);
	core_assert_msg((_size[idx] & 15) == 0, "Size is not aligned properly");
	const BufferType type = _targets[idx];
	const Id id = _handles[idx];
	if (oldSize >= size && _modes[idx] != BufferMode::Static) {
		video::bufferSubData(id, type, 0, data, _size[idx]);
	} else {
		video::bufferData(id, type, _modes[idx], data, _size[idx]);
	}

	return true;
}

int32_t Buffer::create(const void* data, size_t size, BufferType target) {
	if (_handleIdx >= MAX_HANDLES) {
		return -1;
	}
	const int idx = _handleIdx;
	_targets[idx] = target;
	_handles[idx] = video::genBuffer();
	if (!isValid(idx)) {
		Log::error("Failed to create buffer (size: %i)", (int)size);
		return -1;
	}
	_size[idx] = align(size, target);
	if (data != nullptr) {
		update(idx, data, _size[idx]);
	}
	++_handleIdx;
	return idx;
}

int32_t Buffer::createFullscreenQuad() {
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
	alignas(16) static const glm::vec3 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3( 1.0f,  1.0f, 0.0f),
		// left bottom, right top, left top
		glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(-1.0f,  1.0f, 0.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t Buffer::createFullscreenTextureBuffer() {
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
	alignas(16) static const glm::vec2 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f),
		// left bottom, right top, left top
		glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t Buffer::createFullscreenTextureBufferYFlipped() {
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
	alignas(16) static const glm::vec2 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f),
		// left bottom, right top, left top
		glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t Buffer::createWhiteColorForQuad() {
	alignas(16) static const glm::vec4 color[] = {
		glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
	};
	return create(color, sizeof(color));
}

glm::ivec2 Buffer::createTexturedQuad(const glm::ivec2& xy, const glm::ivec2& dimension) {
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
	alignas(16) const glm::vec2 vecs[] = {
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

glm::ivec2 Buffer::createFullscreenTexturedQuad(bool yFlipped) {
	glm::ivec2 indices;
	indices.x = createFullscreenQuad();
	if (yFlipped) {
		indices.y = createFullscreenTextureBufferYFlipped();
	} else {
		indices.y = createFullscreenTextureBuffer();
	}
	return indices;
}

Buffer::~Buffer() {
	core_assert_msg(_vao == InvalidId, "Vertex buffer was not properly shut down");
	shutdown();
}

void Buffer::shutdown() {
	video::deleteVertexArray(_vao);
	video::deleteBuffers(_handleIdx, _handles);
	_handleIdx = 0;
	for (int i = 0; i < MAX_HANDLES; ++i) {
		_targets[i] = BufferType::Max;
		_modes[i] = BufferMode::Static;
		_size[i] = 0u;
	}
	clearAttributes();
}

void Buffer::clearAttributes() {
	_dirtyAttributes = false;
	_attributes.clear();
}

int Buffer::attributes() const {
	return (int)_attributes.size();
}

}
