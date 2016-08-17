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

	static constexpr int CAMERAS = 2;
	// the cameras to render the frustums for
	video::Camera _renderCamera[CAMERAS];
	video::VertexBuffer _frustumBuffer[CAMERAS];
	int32_t _vertexIndex[CAMERAS] = { -1, -1 };
	int32_t _indexIndex[CAMERAS] = { -1, -1 };
	glm::vec4 _colors[CAMERAS];

	int _targetCamera = 0;
	shader::ColorShader _colorShader;

	void doRender() override;
public:
	TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
