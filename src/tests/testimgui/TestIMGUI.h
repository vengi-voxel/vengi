/**
 * @file
 */

#pragma once

#include "imgui/IMGUIApp.h"
#include "imgui/imgui.h"

/**
 * @brief Renders the imgui demo
 */
class TestIMGUI: public imgui::IMGUIApp {
private:
	using Super = imgui::IMGUIApp;
	ImVec4 _clearColor = ImColor(114, 144, 154);
	bool _showTestWindow = false;
	bool _showGraphWindow = true;

public:
	TestIMGUI(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	void onRenderUI() override;
};
