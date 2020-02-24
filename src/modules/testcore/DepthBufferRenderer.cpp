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
#include "core/Log.h"

namespace testcore {

DepthBufferRenderer::DepthBufferRenderer() :
		_shadowMapRenderShader(shader::ShadowmapRenderShader::getInstance()) {
}

bool DepthBufferRenderer::init() {
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmap debug shader");
		return false;
	}

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	_shadowMapDebugBuffer.addAttribute(_shadowMapRenderShader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec3::x));
	_shadowMapDebugBuffer.addAttribute(_shadowMapRenderShader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x));
	return true;
}

void DepthBufferRenderer::shutdown() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
}

void DepthBufferRenderer::renderShadowMap(const video::Camera& camera, const video::FrameBuffer& depthBuffer, int cascade) {
	core_trace_scoped(RenderShadowMap);
	const int frameBufferWidth = camera.frameBufferWidth();
	const int frameBufferHeight = camera.frameBufferHeight();

	// activate shader
	video::ScopedShader scopedShader(_shadowMapRenderShader);
	_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
	_shadowMapRenderShader.setFar(camera.farPlane());
	_shadowMapRenderShader.setNear(camera.nearPlane());

	// bind buffers
	video::ScopedBuffer scopedBuf(_shadowMapDebugBuffer);

	// configure shadow map texture
	const video::TexturePtr& depthTex = depthBuffer.texture(video::FrameBufferAttachment::Depth);
	video::bindTexture(video::TextureUnit::Zero, depthTex);
	video::setupDepthCompareTexture(depthTex->type(), video::CompareFunc::Less, video::TextureCompareMode::None);

	// render shadow map
	// TODO: apply view-projection matrix
	const int halfWidth = (int) (frameBufferWidth / 4.0f);
	const int halfHeight = (int) (frameBufferHeight / 4.0f);
	video::ScopedViewPort scopedViewport(cascade * halfWidth, 0, halfWidth, halfHeight);
	_shadowMapRenderShader.setCascade(cascade);
	video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));

	// restore texture
	video::setupDepthCompareTexture(depthTex->type(), video::CompareFunc::Less, video::TextureCompareMode::RefToTexture);
}

}
