/**
 * @file
 */

#include "TextureAtlasRenderer.h"
#include "video/Renderer.h"

namespace video {

bool TextureAtlasRenderer::init() {
	video::FrameBufferConfig cfg;
	glm::ivec2 dimensions(4096, 4096);
	cfg.dimension(dimensions);
	cfg.colorTexture(true).colorTextureFormat(video::TextureFormat::RGB);
	cfg.depthBuffer(true).depthTextureFormat(video::TextureFormat::D24);
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
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

	const video::TexturePtr& texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	const float texWidth = (float)texture->width();
	const float texHeight = (float)texture->height();
	const float _sx = pos.x;
	const float _sy = pos.y;
	const float _tx = pos.x + w;
	const float _ty = pos.y + h;
	const float sx = _sx / texWidth;
	const float sy = _sy / texHeight;
	const float tx = _tx / texWidth;
	const float ty = _ty / texHeight;

	return TextureAtlasData{sx, sy, tx, ty, texture->width(), texture->height(), texture->handle()};
}

void TextureAtlasRenderer::endRender() {
	// this also restores the viewport
	_frameBuffer.unbind();
}

}