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
	TestDepthBuffer(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
