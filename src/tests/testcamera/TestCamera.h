/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Camera.h"
#include "ColorShader.h"

/**
 * @brief Renders the view frustum of a camera
 */
class TestCamera: public TestApp {
private:
	using Super = TestApp;

	// the camera to render the frustum for
	video::Camera _renderCamera;
	video::VertexBuffer _frustumBuffer;
	int32_t _frustumIndex = -1;
	shader::ColorShader _colorShader;

	void doRender() override;
public:
	TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
