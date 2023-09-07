/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"

/**
 * @brief Renders the imgui demo
 */
class TestIMGUI: public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	bool _showTestWindow = false;
	bool _showMetricsWindow = false;
	bool _showImPlotWindow = false;

public:
	TestIMGUI(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	void onRenderUI() override;
};
