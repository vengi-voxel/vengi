/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Buffer.h"
#include "TestglslgeomShaders.h"

/**
 * @brief Visual test for GLSL geometry shaders
 *
 * This test application is using a geometry shader to build a sphere from a single point.
 */
class TestGLSLGeom: public TestApp {
private:
	using Super = TestApp;
	shader::TestShader _testShader;
	video::Buffer _buffer;
	int _sides = 16;
	float _radius = 10.0f;

	void doRender() override;
public:
	TestGLSLGeom(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual void onRenderUI() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
