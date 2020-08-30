/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "math/Plane.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"

/**
 * @brief Renders a plane
 */
class TestPlane: public TestApp {
private:
	using Super = TestApp;

	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	math::Plane _plane;

	void doRender() override;
public:
	TestPlane(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
