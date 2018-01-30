/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"

/**
 * @brief Renders the imgui demo
 */
class NoiseTool2: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
	bool _windowOpened = true;

public:
	NoiseTool2(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onConstruct() override;
	core::AppState onCleanup() override;
	void onRenderUI() override;
};
