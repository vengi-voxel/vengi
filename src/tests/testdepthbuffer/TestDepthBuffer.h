/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/DepthBuffer.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "video/SunLight.h"
#include "frontend/Plane.h"
#include "FrontendShaders.h"

class TestDepthBuffer: public TestApp {
private:
	using Super = TestApp;
	video::DepthBuffer _depthBuffer;
	video::MeshPtr _mesh;
	video::MeshPool _meshPool;
	shader::MeshShader _meshShader;
	video::VertexBuffer _texturedFullscreenQuad;
	frontend::Plane _plane;
	video::SunLight _sunLight;
	shader::ShadowmapRenderShader _shadowMapRenderShader;
	shader::ShadowmapShader _shadowMapShader;

	void renderPlane();
	void doRender() override;
public:
	TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;

	void onMouseWheel(int32_t x, int32_t y) override;
};
