/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"
#include "video/MeshPool.h"
#include "ui/VoxEditWindow.h"

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 */
class VoxEdit: public ui::turbobadger::UIApp {
private:
	using Super = ui::turbobadger::UIApp;
	core::VarPtr _lastDirectory;
	voxedit::VoxEditWindow* _mainWindow;
	video::MeshPoolPtr _meshPool;
	void update();

public:
	VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::MeshPoolPtr& meshPool);

	std::string fileDialog(OpenFileMode mode, const std::string& filter) override;

	bool importheightmapFile(const std::string& file);
	bool saveFile(const std::string& file);
	bool loadFile(const std::string& file);
	bool voxelizeFile(const std::string& file);
	bool exportFile(const std::string& file);
	bool newFile(bool force = false);
	void selectCursor();
	void select(const glm::ivec3& pos);

	video::MeshPoolPtr meshPool() const;

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};

inline video::MeshPoolPtr VoxEdit::meshPool() const {
	return _meshPool;
}
