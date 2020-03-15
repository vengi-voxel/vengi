/**
 * @file
 */

#include "ShadowmapRenderShader.h"
#include "DepthbufferRenderShader.h"
#include "video/Buffer.h"
#include "video/FrameBuffer.h"

namespace video {
class Camera;
class FrameBuffer;
}

namespace testcore {

class DepthBufferRenderer {
private:
	shader::ShadowmapRenderShader& _shadowMapRenderShader;
	shader::DepthbufferRenderShader& _depthBufferRenderShader;
	video::FrameBuffer _renderToTexture;

	void renderLinearizedDepth(const int frameBufferHeight, const video::FrameBuffer& depthBuffer, const glm::ivec2& xy, const glm::ivec2& wh);

	video::Buffer _shadowMapDebugBuffer;
public:
	DepthBufferRenderer();

	bool init();
	void shutdown();

	void renderShadowMap(const video::Camera& camera, const video::FrameBuffer& depthBuffer, int cascade, const glm::ivec2& xy, const glm::ivec2& wh);
	void renderDepthBuffer(const video::Camera& camera, const video::FrameBuffer& depthBuffer, const glm::ivec2& xy, const glm::ivec2& wh);

	void renderShadowMapToTexture(const video::Camera& camera, const video::FrameBuffer& depthBuffer, int cascade, video::FrameBufferAttachment attachment);
	void renderDepthBufferToTexture(const video::Camera& camera, const video::FrameBuffer& depthBuffer, video::FrameBufferAttachment attachment);

	const video::FrameBuffer& renderToTextureFbo() const;
};

inline const video::FrameBuffer& DepthBufferRenderer::renderToTextureFbo() const {
	return _renderToTexture;
}

}
