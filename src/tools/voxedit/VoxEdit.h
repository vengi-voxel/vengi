/**
 * @file
 */

#pragma once

#include "voxedit-util/SceneManager.h"
#include "core/ArrayLength.h"

#if USE_TURBOBADGER
#include "ui/turbobadger/UIApp.h"
using UIAppType = ui::turbobadger::UIApp;
#else
#include "ui/imgui/IMGUIApp.h"
using UIAppType = ui::imgui::IMGUIApp;
#endif

namespace voxedit {
class VoxEditWindow;
}

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public UIAppType {
private:
	using Super = UIAppType;
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

	void onRenderUI() override;

	void onDropFile(const core::String& file) override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
