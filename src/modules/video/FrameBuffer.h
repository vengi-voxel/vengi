/**
 * @file
 */

#pragma once

#include "FrameBufferConfig.h"
#include <map>
#include <memory>

namespace video {

class Texture;
class RenderBuffer;
typedef std::shared_ptr<Texture> TexturePtr;
typedef std::shared_ptr<RenderBuffer> RenderBufferPtr;

/**
 * @brief A frame buffer is a collection of buffers that can be used as the destination for rendering.
 *
 * @sa FrameBufferConfig
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

	bool bindTextureAttachment(FrameBufferAttachment attachment, int layerIndex, bool clear);
	void bind(bool clear);
	void unbind();

	TexturePtr texture(FrameBufferAttachment attachment = FrameBufferAttachment::Color0) const;

	/**
	 * @return two uv coordinates lower left and upper right (a and c)
	 */
	glm::vec4 uv() const;

	const glm::ivec2& dimension() const;
};

inline const glm::ivec2& FrameBuffer::dimension() const {
	return _dimension;
}

extern bool bindTexture(TextureUnit unit, const FrameBuffer& frameBuffer, FrameBufferAttachment attachment = FrameBufferAttachment::Color0);

}
