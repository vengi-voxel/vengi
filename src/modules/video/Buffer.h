/**
 * @file
 */

#pragma once

#include "Types.h"
#include <glm/vec2.hpp>
#include "core/NonCopyable.h"
#include "core/collection/DynamicArray.h"

#define VIDEO_BUFFER_HASH_COMPARE 0

namespace video {

/**
 * @brief Wrapper for the opengl vertex buffer objects and vertex array objects.
 * @ingroup Video
 */
class Buffer : public core::NonCopyable {
private:
	static constexpr int MAX_HANDLES = 8;
	size_t _size[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
#if VIDEO_BUFFER_HASH_COMPARE
	uint32_t _hash[MAX_HANDLES] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
#endif
	Id _handles[MAX_HANDLES] = {InvalidId, InvalidId, InvalidId, InvalidId, InvalidId, InvalidId, InvalidId, InvalidId};
	BufferType _targets[MAX_HANDLES] = {BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max, BufferType::Max};
	BufferMode _modes[MAX_HANDLES] = {BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static, BufferMode::Static};
	uint32_t _handleIdx = 0u;

	core::DynamicArray<Attribute, 8> _attributes;
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
	void markAttributesDirty();

	/**
	 * This will only destroy the vao - not the buffer handles. Thus a new @c bind() call
	 * will regenerate the vao
	 *
	 * @note This is useful when you are using different graphic contexts
	 */
	void destroyVertexArray();
	bool update(int32_t idx, const void* data, size_t size, bool orphaning = false);

	/**
	 * @return -1 on error - otherwise the index [0,n) of the created buffer (not the Id)
	 */
	int32_t create(const void* data = nullptr, size_t size = 0, BufferType target = BufferType::ArrayBuffer);
	/**
	 * @brief Fullscreen buffer with normalized device coordinates with 3 float components
	 */
	int32_t createFullscreenQuad3d();
	int32_t createFullscreenQuad();

	/**
	 * left, right, top, bottom, back, front
	 *
	 * @note winding order: counter clock wise - front faces inside the cube
	 */
	int32_t createSkyboxQuad();

	/**
	 * @brief Full texture coordinate buffer with 2 float components
	 */
	int32_t createFullscreenTextureBuffer(int32_t idx = -1);
	int32_t createFullscreenTextureBufferYFlipped(int32_t idx = -1);
	/**
	 * @return Two vertex buffers, the first one contains the vertices, the second contains the texcoords
	 */
	glm::ivec2 createFullscreenTexturedQuad3d(bool yFlipped = false);
	glm::ivec2 createFullscreenTexturedQuad(bool yFlipped = false);
	/**
	 * @brief Screen coordinate buffer with 2 float components for vertices and 2 float components for texcoords
	 * @return Two vertex buffers, the first one contains the vertices, the second contains the texcoords
	 */
	glm::ivec2 createTexturedQuad(const glm::ivec2& xy, const glm::ivec2& dimension, bool yFlipped = false);
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

inline void Buffer::markAttributesDirty() {
	_dirtyAttributes = true;
}

inline Id Buffer::handle() const {
	return _vao;
}

class ScopedBuffer {
private:
	const Buffer& _buf;
	bool _success;
public:
	ScopedBuffer(const Buffer& buf) :
			_buf(buf) {
		_success = buf.bind();
	}

	~ScopedBuffer() {
		_buf.unbind();
	}

	inline bool success() const {
		return _success;
	}
};

}
