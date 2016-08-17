/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "frontend/CameraFrustum.h"

/**
 * @brief Renders the view frustum of a camera
 */
class TestCamera: public TestApp {
private:
	using Super = TestApp;

	static constexpr int CAMERAS = 2;
	frontend::CameraFrustum _frustums[CAMERAS];
	// the cameras to render the frustums for
	video::Camera _renderCamera[CAMERAS];

	int _targetCamera = 0;

	void doRender() override;
public:
	TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
