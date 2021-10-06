/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"

class Template : public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;

public:
	Template(const metric::MetricPtr &metric, const core::EventBusPtr &eventBus,
			const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	void onRenderUI() override;
	app::AppState onCleanup() override;
};
