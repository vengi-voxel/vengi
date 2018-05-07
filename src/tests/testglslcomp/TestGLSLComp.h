/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Texture.h"
#include "video/Buffer.h"
#include "TestglslcompShaders.h"
#include "render/TextureRenderer.h"

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
	render::TextureRenderer _renderer;
	video::TexturePtr _texture;

	void doRender() override;
public:
	TestGLSLComp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
