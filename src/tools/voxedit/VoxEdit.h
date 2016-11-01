/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"
#include "ui/MainWindow.h"
#include "video/MeshPool.h"

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class VoxEdit: public ui::UIApp {
private:
	using Super = ui::UIApp;
	core::VarPtr _lastDirectory;
	MainWindow* _mainWindow;
	video::MeshPoolPtr _meshPool;

public:
	VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::MeshPoolPtr& meshPool);

	bool saveFile(std::string_view file);
	bool loadFile(std::string_view file);
	bool voxelizeFile(std::string_view file);
	bool newFile(bool force = false);

	video::MeshPoolPtr meshPool() const;

	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};

inline video::MeshPoolPtr VoxEdit::meshPool() const {
	return _meshPool;
}
