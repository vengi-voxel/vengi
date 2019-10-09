/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"
#include "mesh/MeshPool.h"
#include "ui/VoxEditWindow.h"
#include "voxedit-util/SceneManager.h"
#include "core/Array.h"

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public ui::turbobadger::UIApp {
private:
	using Super = ui::turbobadger::UIApp;
	voxedit::VoxEditWindow* _mainWindow;
	mesh::MeshPoolPtr _meshPool;
	voxedit::SceneManager& _sceneMgr;

public:
	VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const mesh::MeshPoolPtr& meshPool);

	bool importheightmapFile(const std::string& file);
	bool importplaneFile(const std::string& file);
	bool importpaletteFile(const std::string& file);
	bool saveFile(const std::string& file);
	bool loadFile(const std::string& file);
	bool screenshotFile(const std::string& file);
	bool prefabFile(const std::string& file);
	bool importFile(const std::string& file);
	bool exportFile(const std::string& file);
	bool newFile(bool force = false);

	mesh::MeshPoolPtr meshPool() const;

	void onDropFile(const std::string& file) override;

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};

inline mesh::MeshPoolPtr VoxEdit::meshPool() const {
	return _meshPool;
}
