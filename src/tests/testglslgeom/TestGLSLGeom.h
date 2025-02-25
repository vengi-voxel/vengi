/**
 * @file
 */

#pragma once

#include "TestData.h"
#include "testcore/TestApp.h"
#include "video/Buffer.h"
#include "TestShader.h"

/**
 * @brief Visual test for GLSL geometry shaders
 *
 * This test application is using a geometry shader to build a sphere from a single point.
 */
class TestGLSLGeom: public TestApp {
private:
	using Super = TestApp;
	shader::TestData _testData;
	shader::TestShader _testShader;
	video::Buffer _buffer;
	int _sides = 16;
	float _radius = 10.0f;

	void doRender() override;
public:
	TestGLSLGeom(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	virtual void onRenderUI() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
