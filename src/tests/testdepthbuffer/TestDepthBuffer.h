/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/DepthBuffer.h"
#include "video/Mesh.h"
#include "video/VertexBuffer.h"
#include "FrontendShaders.h"

class TestDepthBuffer: public TestApp {
private:
	using Super = TestApp;
	video::DepthBuffer _depthBuffer;
	video::Mesh _mesh;
	shader::MeshShader _meshShader;
	video::VertexBuffer _texturedFullscreenQuad;
	shader::ShadowmapRenderShader _shadowMapRenderShader;
	shader::ShadowmapShader _shadowMapShader;

	void doRender() override;
public:
	TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
