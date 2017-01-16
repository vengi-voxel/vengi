/**
 * @file
 */

#include "GBuffer.h"
#include "ScopedFrameBuffer.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

GBuffer::GBuffer() :
		_fbo(InvalidId), _depthTexture(InvalidId) {
	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		_textures[i] = InvalidId;
	}
}

GBuffer::~GBuffer() {
	core_assert_msg(_fbo == InvalidId, "GBuffer was not properly shut down");
	shutdown();
}

void GBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	const int texCount = (int)SDL_arraysize(_textures);
	video::deleteTextures(texCount, _textures);
	video::deleteTexture(_depthTexture);
}

bool GBuffer::init(const glm::ivec2& dimension) {
	_fbo = video::genFramebuffer();
	ScopedFrameBuffer scopedFrameBuffer(_fbo);

	// +1 for the depth texture
	const int texCount = (int)SDL_arraysize(_textures);
	video::genTextures(texCount + 1, _textures);
	for (std::size_t i = 0; i < texCount; ++i) {
		video::bindTexture(video::TextureUnit::Upload, video::TextureType::Texture2D, _textures[i]);
		// we are going to write vec3 into the out vars in the shaders
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, dimension.x, dimension.y, 0, GL_RGB, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _textures[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	video::bindTexture(video::TextureUnit::Upload, video::TextureType::Texture2D, _depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, dimension.x, dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	static_assert(SDL_arraysize(drawBuffers) == SDL_arraysize(_textures), "buffers and textures don't match");
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void GBuffer::bindForWriting() {
	if (_oldDrawFramebuffer == InvalidId) {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&_oldDrawFramebuffer);
		video::checkError();
	}

	video::bindFramebuffer(FrameBufferMode::Draw, _fbo);
}

void GBuffer::bindForReading(bool gbuffer) {
	if (gbuffer) {
		if (_oldReadFramebuffer == InvalidId) {
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&_oldReadFramebuffer);
			video::checkError();
		}
		bindFramebuffer(FrameBufferMode::Read, _fbo);
		return;
	}

	if (_oldDrawFramebuffer == InvalidId) {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&_oldDrawFramebuffer);
		video::checkError();
	}
	bindFramebuffer(FrameBufferMode::Draw, InvalidId);

	// activate the textures to read from
	const video::TextureUnit texUnits[] = { TextureUnit::One, TextureUnit::Two, TextureUnit::Three };
	static_assert(SDL_arraysize(texUnits) == SDL_arraysize(_textures), "texunits and textures don't match");
	for (int i = 0; i < (int) SDL_arraysize(_textures); ++i) {
		core_assert(_textures[i] != InvalidId);
		video::bindTexture(texUnits[i], video::TextureType::Texture2D, _textures[i]);
	}
}

void GBuffer::unbind() {
	if (_oldDrawFramebuffer != InvalidId) {
		bindFramebuffer(FrameBufferMode::Draw, (Id)_oldDrawFramebuffer);
		_oldDrawFramebuffer = InvalidId;
	}
	if (_oldReadFramebuffer != InvalidId) {
		bindFramebuffer(FrameBufferMode::Draw, (Id)_oldDrawFramebuffer);
		_oldReadFramebuffer = InvalidId;
	}
}

void GBuffer::setReadBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
	video::checkError();
}

}
