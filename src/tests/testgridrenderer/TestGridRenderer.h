/**
 * @file
 */

#pragma once

#include "render/GridRenderer.h"
#include "testcore/TestApp.h"

/**
 * @brief Renders a grid
 */
class TestGridRenderer : public TestApp {
private:
	using Super = TestApp;

	render::GridRenderer _gridRenderer;
	math::AABB<float> _aabb;

	void doRender() override;

public:
	TestGridRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
