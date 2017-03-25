/**
 * @file
 */

#pragma once

#include "imgui/IMGUIApp.h"
#include "imgui/IMGUI.h"

/**
 * @brief Renders the imgui demo
 */
class NoiseTool2: public imgui::IMGUIApp {
private:
	using Super = imgui::IMGUIApp;
	bool _windowOpened = true;

public:
	NoiseTool2(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onConstruct() override;
	core::AppState onCleanup() override;
	void onRenderUI() override;
};
