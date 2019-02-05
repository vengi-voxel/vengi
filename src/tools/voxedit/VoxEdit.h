/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"
#include "video/MeshPool.h"
#include "ui/VoxEditWindow.h"
#include "core/command/ActionButton.h"
#include "core/Array.h"

/**
 * @brief Move directions for the cursor
 */
static const struct Direction {
	const char *postfix;
	int x;
	int y;
	int z;
} DIRECTIONS[] = {
	{"left",     -1,  0,  0},
	{"right",     1,  0,  0},
	{"up",        0,  1,  0},
	{"down",      0, -1,  0},
	{"forward",   0,  0,  1},
	{"backward",  0,  0, -1}
};

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public ui::turbobadger::UIApp {
private:
	using Super = ui::turbobadger::UIApp;
	core::VarPtr _lastDirectory;
	voxedit::VoxEditWindow* _mainWindow;
	video::MeshPoolPtr _meshPool;
	void update();

	core::ActionButton _move[lengthof(DIRECTIONS)];
	uint64_t _lastMove[lengthof(DIRECTIONS)];

public:
	VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::MeshPoolPtr& meshPool);

	bool importheightmapFile(const std::string& file);
	bool saveFile(const std::string& file);
	bool loadFile(const std::string& file);
	bool screenshotFile(const std::string& file);
	bool prefabFile(const std::string& file);
	bool importmeshFile(const std::string& file);
	bool exportFile(const std::string& file);
	bool newFile(bool force = false);

	video::MeshPoolPtr meshPool() const;

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};

inline video::MeshPoolPtr VoxEdit::meshPool() const {
	return _meshPool;
}
