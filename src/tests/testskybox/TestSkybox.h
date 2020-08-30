/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/Skybox.h"
#include "core/Var.h"

/**
 * https://learnopengl.com/Advanced-OpenGL/Cubemaps
 */
class TestSkybox: public TestApp {
private:
	using Super = TestApp;
	render::Skybox _skybox;
	core::VarPtr _skyboxVar;

	void doRender() override;
public:
	TestSkybox(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
