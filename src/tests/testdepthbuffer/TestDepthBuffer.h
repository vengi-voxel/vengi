/**
 * @file
 */

#pragma once

#include "testcore/TestMeshApp.h"

class TestDepthBuffer: public TestMeshApp {
private:
	using Super = TestMeshApp;

	video::VertexBuffer _texturedFullscreenQuad;
	shader::ShadowmapRenderShader _shadowMapRenderShader;

	void doRender() override;
public:
	TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
