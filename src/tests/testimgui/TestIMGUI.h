/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"

/**
 * @brief Renders the imgui demo
 */
class TestIMGUI: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
	bool _showTestWindow = false;
	bool _showMetricsWindow = false;

public:
	TestIMGUI(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	app::AppState onInit() override;
	void onRenderUI() override;
};
