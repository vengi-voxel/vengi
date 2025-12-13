/**
 * @file
 */

#include "Buffer.h"
#include "Renderer.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/Trace.h"
#if VIDEO_BUFFER_HASH_COMPARE
#include "core/Hash.h"
#endif

namespace video {

Buffer::Buffer(const void* data, size_t size, BufferType target) :
		_attributes(32) {
	create(data, size, target);
}

Buffer::Buffer() {
}

size_t Buffer::align(size_t x, BufferType type) {
	size_t a = 32;
	switch (type) {
	case BufferType::IndexBuffer:
		a = 16;
		break;
	case BufferType::UniformBuffer:
		a = video::specificationi(Spec::UniformBufferAlignment);
		break;
	default:
		break;
	}
	return ( ( ( x ) + ((a)-1) ) & ~((a)-1) );
}

bool Buffer::isValid(int32_t idx) const {
	if (idx < 0) {
		return false;
	}
	if (idx >= MAX_HANDLES) {
		return false;
	}
	return _handles[idx] != InvalidId;
}

uint32_t Buffer::size(int32_t idx) const {
	core_assert_msg(idx >= 0 && idx < MAX_HANDLES, "Given index %i is out of range", idx);
	return (uint32_t)_size[idx];
}

uint32_t Buffer::elements(int32_t idx, int components, size_t componentSize) const {
	const uint32_t s = size(idx);
	return s / (components * (uint32_t)componentSize);
}

Id Buffer::bufferHandle(int32_t idx) const {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	return _handles[idx];
}

void Buffer::setMode(int32_t idx, BufferMode mode) {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	_modes[idx] = mode;
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
	_attributes.emplace_back(attribute);
	_dirtyAttributes = true;
	return true;
}

void Buffer::destroyVertexArray() {
	video::deleteVertexArray(_vao);
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

	for (const Attribute& a : _attributes) {
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

bool Buffer::update(int32_t idx, const void* data, size_t size, bool orphaning) {
	if (!isValid(idx)) {
		return false;
	}
	core_trace_scoped(UpdateBuffer);

	core_assert(video::boundVertexArray() == InvalidId);
	const size_t oldSize = _size[idx];
#if VIDEO_BUFFER_HASH_COMPARE
	if (oldSize == size) {
		uint32_t newHash = core::hash(data, size);
		if (newHash == _hash[idx]) {
			return true;
		}
		_hash[idx] = newHash;
	} else {
		_hash[idx] = core::hash(data, size);
	}
#endif
	_size[idx] = size;
#ifndef __EMSCRIPTEN__
	core_assert_16byte_aligned(data);
#endif
	//core_assert_msg((_size[idx] & 15) == 0, "Size is not aligned properly: %i", (int)_size[idx]);
	const BufferType type = _targets[idx];
	const Id id = _handles[idx];
	if (size > 0) {
		// If the requested update fits into the old allocation and the buffer
		// isn't static, prefer sub-data path if mapping isn't possible.
		if (oldSize >= size && _modes[idx] != BufferMode::Static) {
			// try map range
			void *ptr = video::mapBufferRange(id, type, 0, size, video::AccessMode::Write,
											  video::MapBufferFlag::Unsynchronized | video::MapBufferFlag::InvalidateRange);
			if (ptr != nullptr) {
				core_memcpy(ptr, data, size);
				// explicit flush isn't required in this path; unmap will commit
				video::unmapBuffer(id, type);
			} else {
				video::bufferSubData(id, type, 0, data, size);
			}
		} else {
			if (orphaning) {
				// ensure buffer has required size
				if (oldSize < size) {
					video::bufferData(id, type, _modes[idx], nullptr, size);
				}
				void *ptr = video::mapBufferRange(id, type, 0, size, video::AccessMode::Write,
												  video::MapBufferFlag::Unsynchronized | video::MapBufferFlag::InvalidateRange);
				if (ptr != nullptr) {
					core_memcpy(ptr, data, size);
					video::unmapBuffer(id, type);
				} else {
					video::bufferSubData(id, type, 0, data, size);
				}
			} else {
				video::bufferData(id, type, _modes[idx], data, size);
			}
		}
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
	_size[idx] = size;
	if (data != nullptr) {
		update(idx, data, size);
	}
	++_handleIdx;
	return idx;
}

int32_t Buffer::createSkyboxQuad() {
	/**
	 *               0/1/-1
	 *     ------------------ 1/1/-1
	 *    /|       /       /|
	 *   /--------/0/1/0--/ |
	 *  -1/1/1   /       /  |
	 *  -----------------1/1/1
	 *  | /|    |       |  /|
	 *  |/ |    |0/0/1  | / |
	 *  -----------------   / 1/-1/-1
	 *  | /     |       |  /
	 *  |/      |       | /
	 *  -----------------
	 *  -1/-1/1          1/-1/1
	 */
	alignas(16) static const glm::vec3 vecs[] = {
		// side: right
		{ 1.0f, -1.0f, -1.0f}, // tri(1): right, bottom, front
		{ 1.0f, -1.0f,  1.0f}, // tri(1): right, bottom, back
		{ 1.0f,  1.0f,  1.0f}, // tri(1): right, bottom, back
		{ 1.0f,  1.0f,  1.0f}, // tri(2): right, bottom, back
		{ 1.0f,  1.0f, -1.0f}, // tri(2): right, top,    front
		{ 1.0f, -1.0f, -1.0f}, // tri(2): right, bottom, front

		// side: left
		{-1.0f, -1.0f,  1.0f}, // tri(3): left, bottom, back
		{-1.0f, -1.0f, -1.0f}, // tri(3): left, bottom, front
		{-1.0f,  1.0f, -1.0f}, // tri(3): left, top,    front
		{-1.0f,  1.0f, -1.0f}, // tri(4): left, top,    front
		{-1.0f,  1.0f,  1.0f}, // tri(4): left, top,    back
		{-1.0f, -1.0f,  1.0f}, // tri(4): left, bottom, back

		// side: top
		{-1.0f,  1.0f, -1.0f}, // tri(5): left,  top, front
		{ 1.0f,  1.0f, -1.0f}, // tri(5): right, top, front
		{ 1.0f,  1.0f,  1.0f}, // tri(5): right, top, back
		{ 1.0f,  1.0f,  1.0f}, // tri(6): right, top, back
		{-1.0f,  1.0f,  1.0f}, // tri(6): left,  top, back
		{-1.0f,  1.0f, -1.0f}, // tri(6): left,  top, front

		// side: bottom
		{-1.0f, -1.0f, -1.0f}, // tri(7): left,  bottom, front
		{-1.0f, -1.0f,  1.0f}, // tri(7): left,  bottom, back
		{ 1.0f, -1.0f, -1.0f}, // tri(7): right, bottom, front
		{ 1.0f, -1.0f, -1.0f}, // tri(8): right, bottom, front
		{-1.0f, -1.0f,  1.0f}, // tri(8): left,  bottom, back
		{ 1.0f, -1.0f,  1.0f}, // tri(8): right, bottom, back

		// side: back
		{-1.0f, -1.0f,  1.0f}, // tri(9):  left,  bottom, back
		{-1.0f,  1.0f,  1.0f}, // tri(9):  left,  top,    back
		{ 1.0f,  1.0f,  1.0f}, // tri(9):  right, top,    back
		{ 1.0f,  1.0f,  1.0f}, // tri(10): right, top,    back
		{ 1.0f, -1.0f,  1.0f}, // tri(10): right, bottom, back
		{-1.0f, -1.0f,  1.0f}, // tri(10): left,  bottom, back

		// side: front
		{-1.0f,  1.0f, -1.0f}, // tri(11): left,  top,    front
		{-1.0f, -1.0f, -1.0f}, // tri(11): left,  bottom, front
		{ 1.0f, -1.0f, -1.0f}, // tri(11): right, bottom, front
		{ 1.0f, -1.0f, -1.0f}, // tri(12): right, bottom, front
		{ 1.0f,  1.0f, -1.0f}, // tri(12): right, top,    front
		{-1.0f,  1.0f, -1.0f}  // tri(12): left,  top,    front
	};
	return create(vecs, sizeof(vecs));
}

int32_t Buffer::createFullscreenQuad3d() {
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
	alignas(16) static const glm::vec2 vecs[] = {
		// left bottom, right bottom, right top
		glm::vec2(-1.0f, -1.0f), glm::vec2( 1.0f, -1.0f), glm::vec2( 1.0f,  1.0f),
		// left bottom, right top, left top
		glm::vec2(-1.0f, -1.0f), glm::vec2( 1.0f,  1.0f), glm::vec2(-1.0f,  1.0f)
	};
	return create(vecs, sizeof(vecs));
}

int32_t Buffer::createFullscreenTextureBuffer(int32_t idx) {
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
	if (idx == -1) {
		return create(vecs, sizeof(vecs));
	}
	update(idx, vecs, sizeof(vecs));
	return idx;
}

int32_t Buffer::createFullscreenTextureBufferYFlipped(int32_t idx) {
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
	if (idx == -1) {
		return create(vecs, sizeof(vecs));
	}
	update(idx, vecs, sizeof(vecs));
	return idx;
}

int32_t Buffer::createWhiteColorForQuad() {
	alignas(16) static const glm::vec4 color[] = {
		glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
	};
	return create(color, sizeof(color));
}

glm::ivec2 Buffer::createTexturedQuad(const glm::ivec2& xy, const glm::ivec2& dimension, bool yFlipped) {
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
	if (yFlipped) {
		indices.y = createFullscreenTextureBufferYFlipped();
	} else {
		indices.y = createFullscreenTextureBuffer();
	}
	return indices;
}

glm::ivec2 Buffer::createFullscreenTexturedQuad3d(bool yFlipped) {
	glm::ivec2 indices;
	indices.x = createFullscreenQuad3d();
	if (yFlipped) {
		indices.y = createFullscreenTextureBufferYFlipped();
	} else {
		indices.y = createFullscreenTextureBuffer();
	}
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
	_attributes.release();
}

int Buffer::attributes() const {
	return (int)_attributes.size();
}

}
