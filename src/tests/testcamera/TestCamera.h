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

	static constexpr int CAMERAS = 3;
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

	void afterUI() override;

	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	void onMouseWheel(int32_t x, int32_t y) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};
