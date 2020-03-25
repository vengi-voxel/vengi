/**
 * @file
 */

#include "TextureAtlasRenderer.h"
#include "video/Renderer.h"

namespace video {

bool TextureAtlasRenderer::init() {
	video::TextureConfig textureCfg;
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig cfg;
	glm::ivec2 dimensions(4096, 4096);
	cfg.dimension(dimensions);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	return _frameBuffer.init(cfg);
}

void TextureAtlasRenderer::shutdown() {
	_frameBuffer.shutdown();
}

glm::ivec2 TextureAtlasRenderer::resolvePos(int id, int w, int h) {
	// TODO: find free (but if already used - see given id - reuse the existing) slot
	// to render to the framebuffer texture
	return glm::ivec2(0);
}

TextureAtlasData TextureAtlasRenderer::beginRender(int id, int w, int h) {
	_frameBuffer.bind(false);
	const glm::ivec2& pos = resolvePos(id, w, h);
	// update the viewport to the target rect of the texture
	video::viewport(pos.x, pos.y, w, h);
	const video::TexturePtr& texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	return TextureAtlasData{pos.x, pos.y, w, h, texture->width(), texture->height(), texture->handle()};
}

void TextureAtlasRenderer::endRender() {
	// this also restores the viewport
	_frameBuffer.unbind();
}

}