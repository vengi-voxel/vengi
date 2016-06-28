/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/DepthBuffer.h"
#include "video/Mesh.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "FrontendShaders.h"

class TestDepthBuffer: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
	video::DepthBuffer _depthBuffer;
	video::Mesh _mesh;
	video::Camera _camera;
	shader::MeshShader _meshShader;
	video::VertexBuffer _texturedFullscreenQuad;
	shader::ShadowmapRenderShader _shadowMapRenderShader;
	shader::ShadowmapShader _shadowMapShader;
public:
	TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~TestDepthBuffer();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
