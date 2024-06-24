/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/CameraFrustum.h"
#include "FrustumEntity.h"
#include "core/collection/Array.h"

/**
 * @brief Renders the view frustum of a camera
 */
class TestCamera: public TestApp {
private:
	using Super = TestApp;

	static constexpr int CAMERAS = 3;
	render::CameraFrustum _frustums[CAMERAS];
	// the cameras to render the frustums for
	video::Camera _renderCamera[CAMERAS];

	core::Array<FrustumEntity, 25> _entities;

	int _targetCamera = 0;

	void doRender() override;
	void resetCameraPosition();
public:
	TestCamera(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;

	void onRenderUI() override;

	bool onMouseWheel(float x, float y) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};
