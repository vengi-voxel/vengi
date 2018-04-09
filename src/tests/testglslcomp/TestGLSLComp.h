/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Texture.h"
#include "video/VertexBuffer.h"
#include "TestglslcompShaders.h"
#include "FrontendShaders.h"

/**
 * @brief Visual test for GLSL compute shaders
 *
 * This test application is using a compute shader to fill
 * a texture that is rendered later on.
 */
class TestGLSLComp: public TestApp {
private:
	using Super = TestApp;
	shader::TestShader _testShader;
	video::TexturePtr _texture;
	shader::TextureShader _textureShader;
	video::VertexBuffer _texturedFullscreenQuad;

	void doRender() override;
public:
	TestGLSLComp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
