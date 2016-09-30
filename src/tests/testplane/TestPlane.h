/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "core/Plane.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

/**
 * @brief Renders a plane
 */
class TestPlane: public TestApp {
private:
	using Super = TestApp;

	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	core::Plane _plane;

	void doRender() override;
public:
	TestPlane(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
