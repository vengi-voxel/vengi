/**
 * @file
 */

#include "GBuffer.h"
#include "ScopedFrameBuffer.h"
#include "Types.h"

#include <stddef.h>
#include "core/Common.h"
#include "core/Assert.h"

namespace video {

GBuffer::GBuffer() :
		_fbo(InvalidId), _depthTexture(InvalidId) {
	for (size_t i = 0u; i < SDL_arraysize(_textures); ++i) {
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

	// +1 for the depth texture
	const int texCount = (int)SDL_arraysize(_textures);
	video::genTextures(texCount + 1, _textures);

	return video::setupGBuffer(_fbo, dimension, _textures, SDL_arraysize(_textures), _depthTexture);
}

void GBuffer::bindForWriting() {
	video::bindFramebuffer(_fbo, FrameBufferMode::Draw);
}

void GBuffer::bindForReading(bool gbuffer) {
	if (gbuffer) {
		video::bindFramebuffer(_fbo, FrameBufferMode::Read);
		return;
	}

	video::bindFramebuffer(InvalidId, FrameBufferMode::Draw);

	// activate the textures to read from
	const video::TextureUnit texUnits[] = { TextureUnit::Zero, TextureUnit::One, TextureUnit::Two };
	static_assert(SDL_arraysize(texUnits) == SDL_arraysize(_textures), "texunits and textures don't match");
	for (int i = 0; i < (int) SDL_arraysize(_textures); ++i) {
		video::bindTexture(texUnits[i], video::TextureType::Texture2D, _textures[i]);
	}
	video::activateTextureUnit(video::TextureUnit::Zero);
}

void GBuffer::unbind() {
	video::bindFramebuffer(InvalidId, FrameBufferMode::Default);
}

void GBuffer::setReadBuffer(GBufferTextureType textureType) {
	video::readBuffer(textureType);
}

}
