/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <map>
#include <memory>

namespace video {

typedef std::shared_ptr<Texture> TexturePtr;
typedef std::shared_ptr<RenderBuffer> RenderBufferPtr;

/**
 * @ingroup Video
 */
class FrameBuffer {
	friend class ScopedFrameBuffer;
private:
	ClearFlag _clearFlag = ClearFlag::None;
	Id _fbo = video::InvalidId;
	Id _oldFramebuffer = video::InvalidId;
	std::map<FrameBufferAttachment, TexturePtr> _colorAttachments;
	std::map<FrameBufferAttachment, RenderBufferPtr> _bufferAttachments;

	glm::ivec2 _dimension;

	int32_t _viewport[4] = {0, 0, 0, 0};

	bool prepareAttachments(const FrameBufferConfig& cfg);
public:
	~FrameBuffer();

	bool init(const FrameBufferConfig& cfg);
	void shutdown();

	void bind(bool clear = true);
	void unbind();

	Id texture() const;

	/**
	 * @return two uv coordinates lower left and upper right (a and c)
	 */
	glm::vec4 uv() const;

	const glm::ivec2& dimension() const;
};

inline const glm::ivec2& FrameBuffer::dimension() const {
	return _dimension;
}

}
