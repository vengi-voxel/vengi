/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"

/**
 * @brief Renders the imgui demo
 */
class TestIMGUI: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
	bool _showTestWindow = false;
	bool _showMetricsWindow = false;

public:
	TestIMGUI(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	void onRenderUI() override;
};
