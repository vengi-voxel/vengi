/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include "core/Vector.h"
#include "core/GLM.h"
#include <vector>

namespace video {

/**
 * @brief Wrapper for the opengl vertex buffer objects and vertex array objects.
 * @ingroup Video
 */
class VertexBuffer {
private:
	static constexpr int MAX_HANDLES = 6;
	size_t _size[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u};
	Id _handles[MAX_HANDLES] = {InvalidId, InvalidId, InvalidId, InvalidId, InvalidId, InvalidId};
	VertexBufferType _targets[MAX_HANDLES] = {VertexBufferType::Max, VertexBufferType::Max, VertexBufferType::Max, VertexBufferType::Max, VertexBufferType::Max, VertexBufferType::Max};
	VertexBufferMode _modes[MAX_HANDLES] = {VertexBufferMode::Static, VertexBufferMode::Static, VertexBufferMode::Static, VertexBufferMode::Static, VertexBufferMode::Static, VertexBufferMode::Static};
	uint32_t _handleIdx = 0u;

	std::vector<Attribute> _attributes;
	mutable Id _vao = InvalidId;
	mutable bool _dirtyAttributes = true;
public:
	/**
	 * @brief Ctor that also creates buffer handle.
	 * @note Keep in mind that you need a valid context for this constructor.
	 */
	VertexBuffer(const void* data, size_t size, VertexBufferType target = VertexBufferType::ArrayBuffer);
	/**
	 * @brief Ctor that doesn't create the underlying buffer
	 * @note This ctor can be used to put this as members to other classes.
	 */
	VertexBuffer();
	~VertexBuffer();

	void clearAttributes();
	int attributes() const;

	void setMode(int32_t idx, VertexBufferMode mode);
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
	inline int32_t create(const std::vector<T>& data, VertexBufferType target = VertexBufferType::ArrayBuffer) {
		return create(&data.front(), core::vectorSize(data), target);
	}

	/**
	 * @return -1 on error - otherwise the index [0,n) of the created buffer (not the Id)
	 */
	int32_t create(const void* data = nullptr, size_t size = 0, VertexBufferType target = VertexBufferType::ArrayBuffer);
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

inline Id VertexBuffer::handle() const {
	return _vao;
}

inline bool VertexBuffer::isValid(int32_t idx) const {
	if (idx < 0) {
		return false;
	}
	if (idx >= MAX_HANDLES) {
		return false;
	}
	return _handles[idx] != InvalidId;
}

inline uint32_t VertexBuffer::size(int32_t idx) const {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	return _size[idx];
}

inline uint32_t VertexBuffer::elements(int32_t idx, int components, size_t componentSize) const {
	return size(idx) / (components * componentSize);
}

inline Id VertexBuffer::bufferHandle(int32_t idx) const {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	return _handles[idx];
}

inline void VertexBuffer::setMode(int32_t idx, VertexBufferMode mode) {
	core_assert(idx >= 0 && idx < MAX_HANDLES);
	_modes[idx] = mode;
}

class ScopedVertexBuffer {
private:
	const VertexBuffer& _buf;
public:
	ScopedVertexBuffer(const VertexBuffer& buf) :
			_buf(buf) {
		buf.bind();
	}

	~ScopedVertexBuffer() {
		_buf.unbind();
	}
};

}
