/**
 * @file
 */

#include "DepthBufferRenderer.h"
#include "core/Trace.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/Texture.h"
#include "video/Renderer.h"
#include "video/ScopedViewPort.h"
#include "video/Buffer.h"
#include "core/ArrayLength.h"
#include "core/Log.h"

namespace testcore {

DepthBufferRenderer::DepthBufferRenderer() :
		_shadowMapRenderShader(shader::ShadowmapRenderShader::getInstance()),
		_depthBufferRenderShader(shader::DepthbufferRenderShader::getInstance()) {
}

bool DepthBufferRenderer::init() {
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmap debug shader");
		return false;
	}

	if (!_depthBufferRenderShader.setup()) {
		Log::error("Failed to init depthbuffer debug shader");
		return false;
	}

	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGBA);
	video::FrameBufferConfig fboCfg;
	fboCfg.dimension(glm::ivec2(1024, 1024));
	const video::FrameBufferAttachment attachments[] = {
		video::FrameBufferAttachment::Color0,
		video::FrameBufferAttachment::Color1,
		video::FrameBufferAttachment::Color2,
		video::FrameBufferAttachment::Color3,
		video::FrameBufferAttachment::Color4,
		video::FrameBufferAttachment::Color5
	};
	for (int i = 0; i < lengthof(attachments); ++i) {
		fboCfg.addTextureAttachment(textureCfg, attachments[i]);
	}
	_renderToTexture.init(fboCfg);

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	_shadowMapDebugBuffer.addAttribute(_shadowMapRenderShader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec3::x));
	_shadowMapDebugBuffer.addAttribute(_shadowMapRenderShader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x));
	return true;
}

void DepthBufferRenderer::shutdown() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_depthBufferRenderShader.shutdown();
	_renderToTexture.shutdown();
}

void DepthBufferRenderer::renderLinearizedDepth(const int frameBufferHeight, const video::FrameBuffer& depthBuffer, const glm::ivec2& xy, const glm::ivec2& wh) {
	// bind buffers
	video::ScopedBuffer scopedBuf(_shadowMapDebugBuffer);

	// configure shadow map texture
	const video::TexturePtr& depthTex = depthBuffer.texture(video::FrameBufferAttachment::Depth);
	video::bindTexture(video::TextureUnit::Zero, depthTex);
	video::setupDepthCompareTexture(depthTex->type(), video::CompareFunc::Less, video::TextureCompareMode::None);

	// render depth buffer texture
	video::ScopedViewPort scopedViewport(xy.x, frameBufferHeight - xy.y - wh.y, wh.x, wh.y);
	video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));

	// restore texture
	video::setupDepthCompareTexture(depthTex->type(), video::CompareFunc::Less, video::TextureCompareMode::RefToTexture);
}

void DepthBufferRenderer::renderDepthBuffer(const video::Camera& camera, const video::FrameBuffer& depthBuffer, const glm::ivec2& xy, const glm::ivec2& wh) {
	core_trace_scoped(RenderDepthBuffer);

	video::ScopedShader scopedShader(_depthBufferRenderShader);
	if (_depthBufferRenderShader.isDirty()) {
		_depthBufferRenderShader.setDepthbuffer(video::TextureUnit::Zero);
		_depthBufferRenderShader.markClean();
	}
	_depthBufferRenderShader.setFar(camera.farPlane());
	_depthBufferRenderShader.setNear(camera.nearPlane());

	renderLinearizedDepth(camera.frameBufferHeight(), depthBuffer, xy, wh);
}

void DepthBufferRenderer::renderShadowMap(const video::Camera& camera, const video::FrameBuffer& depthBuffer, int cascade, const glm::ivec2& xy, const glm::ivec2& wh) {
	core_trace_scoped(RenderShadowMap);

	video::ScopedShader scopedShader(_shadowMapRenderShader);
	if (_shadowMapRenderShader.isDirty()) {
		_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
		_shadowMapRenderShader.markClean();
	}
	_shadowMapRenderShader.setFar(camera.farPlane());
	_shadowMapRenderShader.setNear(camera.nearPlane());
	_shadowMapRenderShader.setCascade(cascade);

	renderLinearizedDepth(camera.frameBufferHeight(), depthBuffer, xy, wh);
}

void DepthBufferRenderer::renderShadowMapToTexture(const video::Camera& camera, const video::FrameBuffer& depthBuffer, int cascade, video::FrameBufferAttachment attachment) {
	core_trace_scoped(RenderShadowMapToTexture);
	_renderToTexture.bind(false);
	_renderToTexture.bindTextureAttachment(attachment, 0, false);

	video::ScopedShader scopedShader(_shadowMapRenderShader);
	if (_shadowMapRenderShader.isDirty()) {
		_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
		_shadowMapRenderShader.markClean();
	}
	_shadowMapRenderShader.setFar(camera.farPlane());
	_shadowMapRenderShader.setNear(camera.nearPlane());
	_shadowMapRenderShader.setCascade(cascade);

	const video::TexturePtr& texture = _renderToTexture.texture(attachment);
	const int height = texture->height();
	const int width = texture->width();
	renderLinearizedDepth(_renderToTexture.texture(attachment)->height(), depthBuffer, glm::ivec2(0), glm::ivec2(width, height));
	_renderToTexture.unbind();
}

void DepthBufferRenderer::renderDepthBufferToTexture(const video::Camera& camera, const video::FrameBuffer& depthBuffer, video::FrameBufferAttachment attachment) {
	core_trace_scoped(RenderDepthBufferTexture);
	_renderToTexture.bind(false);
	_renderToTexture.bindTextureAttachment(attachment, 0, false);

	video::ScopedShader scopedShader(_depthBufferRenderShader);
	if (_depthBufferRenderShader.isDirty()) {
		_depthBufferRenderShader.setDepthbuffer(video::TextureUnit::Zero);
		_depthBufferRenderShader.markClean();
	}
	_depthBufferRenderShader.setFar(camera.farPlane());
	_depthBufferRenderShader.setNear(camera.nearPlane());

	const video::TexturePtr& texture = _renderToTexture.texture(attachment);
	const int height = texture->height();
	const int width = texture->width();
	renderLinearizedDepth(_renderToTexture.texture(attachment)->height(), depthBuffer, glm::ivec2(0), glm::ivec2(width, height));
	_renderToTexture.unbind();
}

}
