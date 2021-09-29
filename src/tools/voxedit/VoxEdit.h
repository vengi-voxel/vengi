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

	bool importheightmapFile(const core::String& file);
	bool importplaneFile(const core::String& file);
	bool importpaletteFile(const core::String& file);
	bool saveFile(const core::String& file);
	bool loadFile(const core::String& file);
	bool screenshotFile(const core::String& file);
	bool prefabFile(const core::String& file);
	bool newFile(bool force = false);

	void onRenderUI() override;

	void onDropFile(const core::String& file) override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
