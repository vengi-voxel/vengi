/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"

/**
 * @brief Renders the imgui demo
 */
class TestIMGUI: public TestApp {
private:
	using Super = TestApp;
	bool _showTestWindow = false;
	bool _showMetricsWindow = false;
	bool _showImPlotWindow = false;

	void doRender() override;

public:
	TestIMGUI(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	void onRenderUI() override;
};
