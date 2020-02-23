/**
 * @file
 */

#include "ShadowmapRenderShader.h"
#include "video/Buffer.h"

namespace video {
class Camera;
class FrameBuffer;
}

namespace testcore {

class DepthBufferRenderer {
private:
	shader::ShadowmapRenderShader& _shadowMapRenderShader;
	video::Buffer _shadowMapDebugBuffer;
public:
	DepthBufferRenderer();

	bool init();
	void shutdown();

	void renderShadowMap(const video::Camera& camera, const video::FrameBuffer& frameBuffer, int cascade);
};

}