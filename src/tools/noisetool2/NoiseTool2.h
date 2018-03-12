/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 *
 * @ingroup Tools
 */
class NoiseTool2: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
	bool _windowOpened = true;

public:
	NoiseTool2(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
	void onRenderUI() override;
};
