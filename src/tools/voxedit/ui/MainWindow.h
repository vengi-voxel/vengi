/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "core/String.h"

class EditorScene;
class VoxEdit;

/**
 * @brief Voxel editing tools panel
 */
class MainWindow: public ui::Window {
private:
	EditorScene* _scene;
	VoxEdit* _voxedit;

	std::string _voxelizeFile;
	std::string _loadFile;

	tb::TBCheckBox *_showGrid = nullptr;
	tb::TBCheckBox *_showAABB = nullptr;
	tb::TBCheckBox *_showAxis = nullptr;

	bool handleClickEvent(const tb::TBWidgetEvent &ev);
	bool handleChangeEvent(const tb::TBWidgetEvent &ev);
public:
	MainWindow(VoxEdit* tool);

	bool init();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;

	bool voxelize(std::string_view file);
	bool save(std::string_view file);
	bool load(std::string_view file);
	bool createNew(bool force);

};
