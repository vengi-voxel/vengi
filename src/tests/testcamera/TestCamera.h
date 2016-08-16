/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Camera.h"

/**
 * @brief Renders the view frustum of a camera
 */
class TestCamera: public TestApp {
private:
	using Super = TestApp;

	// the camera to render the frustum for
	video::Camera _renderCamera;

	void doRender() override;
public:
	TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
};
