/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"
#include "voxedit-ui/VoxEditWindow.h"
#include "voxedit-util/SceneManager.h"
#include "core/ArrayLength.h"

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public ui::turbobadger::UIApp {
private:
	using Super = ui::turbobadger::UIApp;
	voxedit::VoxEditWindow* _mainWindow = nullptr;

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

	void onDropFile(const core::String& file) override;

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
