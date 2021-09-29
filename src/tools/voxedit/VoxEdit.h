/**
 * @file
 */

#pragma once

#include "voxedit-util/SceneManager.h"
#include "core/ArrayLength.h"

#include "ui/imgui/IMGUIApp.h"

namespace voxedit {
class MainWindow;
}

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
	voxedit::MainWindow* _mainWindow = nullptr;

public:
	VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void onRenderUI() override;

	void onDropFile(const core::String& file) override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
