/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "math/Plane.h"
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
	TestPlane(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
