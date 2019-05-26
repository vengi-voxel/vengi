/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include "core/Vector.h"
#include "core/GLM.h"
#include <vector>

#define VIDEO_BUFFER_HASH_COMPARE 0

namespace video {

/**
 * @brief Wrapper for the opengl vertex buffer objects and vertex array objects.
 * @ingroup Video
 */
class Buffer {
private:
	static constexpr int MAX_HANDLES = 6;
	size_t _size[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u};
#if VIDEO_BUFFER_HASH_COMPARE
	uint32_t _hash[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u};
#endif
	Id _handles[MAX_HANDLES] = {InvalidId, InvalidId, InvalidId, InvalidId, InvalidId, InvalidId};
	BufferType _targets[MAX_HANDLES] = {BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max};
	BufferMode _modes[MAX_HANDLES] = {BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static};
	uint32_t _handleIdx = 0u;

	std::vector<Attribute> _attributes;
	mutable Id _vao = InvalidId;
	mutable bool _dirtyAttributes = true;

	static size_t align(size_t x, BufferType type);
public:
	/**
	 * @brief Ctor that also creates buffer handle.
	 * @note Keep in mind that you need a valid context for this constructor.
	 */
	Buffer(const void* data, size_t size, BufferType target = BufferType::ArrayBuffer);
	/**
	 * @brief Ctor that doesn't create the underlying buffer
	 * @note This ctor can be used to put this as members to other classes.
	 */
	Buffer();
	~Buffer();

	void clearAttributes();
	int attributes() const;

	void setMode(int32_t idx, BufferMode mode);
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

	/**
	 * @return The size on the gpu
	 * @note The buffer behind the given index must be bound
	 * @sa size()
	 */
	size_t bufferSize(int32_t idx) const;

	template<class T>
	T* data(int32_t idx, video::AccessMode mode) const {
		return (T*)mapData(idx, mode);
	}

	void* mapData(int32_t idx, video::AccessMode mode) const;
	void unmapData(int32_t idx) const;

	bool update(int32_t idx, const void* data, size_t size);

	/**
	 * @return -1 on error - otherwise the index [0,n) of the created buffer (not the Id)
	 */
	template<class T>
	inline int32_t create(const std::vector<T>& data, BufferType target = BufferType::ArrayBuffer) {
		return create(&data.front(), core::vectorSize(data), target);
	}

	/**
	 * @return -1 on error - otherwise the index [0,n) of the created buffer (not the Id)
	 */
	int32_t create(const void* data = nullptr, size_t size = 0, BufferType target = BufferType::ArrayBuffer);
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
	/**
	 * @brief Bind the underlying gl buffers
	 *
	 * @note Also sets up the vertex attributes if this wasn't done before (only done once).
	 * @sa unbind()
	 * @return @c true if the bind was sucessful, @c false otherwise.
	 */
	bool bind() const;
	/**
	 * @sa bind()
	 */
	bool unbind() const;
	/**
	 * @return The size of the cpu allocated memory
	 * @sa bufferSize()
	 */
	uint32_t size(int32_t idx) const;
	uint32_t elements(int32_t idx, int components = 3, size_t componentSize = sizeof(float)) const;
	/**
	 * @param[in] idx The buffer index returned by create()
	 * @return @c true if the index is valid and the buffer for the given index is valid
	 */
	bool isValid(int32_t idx) const;
	/**
	 * @param[in] idx The buffer index returned by create()
	 * @return The native handle for the given buffer index.
	 */
	Id bufferHandle(int32_t idx) const;

	/**
	 * @return The vertex array object native handle
	 */
	Id handle() const;
};

inline size_t Buffer::align(size_t x, BufferType type) {
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

inline Id Buffer::handle() const {
	return _vao;
}

inline bool Buffer::isValid(int32_t idx) const {
	if (idx < 0) {
		return false;
	}
	if (idx >= MAX_HANDLES) {
		return false;
	}
	return _handles[idx] != InvalidId;
}

inline uint32_t Buffer::size(int32_t idx) const {
	core_assert_msg(idx >= 0 && idx < MAX_HANDLES, "Given index %i is out of range", idx);
	return _size[idx];
}

inline uint32_t Buffer::elements(int32_t idx, int components, size_t componentSize) const {
	return size(idx) / (components * componentSize);
}

inline Id Buffer::bufferHandle(int32_t idx) const {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	return _handles[idx];
}

inline void Buffer::setMode(int32_t idx, BufferMode mode) {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	_modes[idx] = mode;
}

class ScopedBuffer {
private:
	const Buffer& _buf;
public:
	ScopedBuffer(const Buffer& buf) :
			_buf(buf) {
		buf.bind();
	}

	~ScopedBuffer() {
		_buf.unbind();
	}
};

}
