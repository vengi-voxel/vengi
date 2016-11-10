/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "core/String.h"

class EditorScene;
class VoxEdit;
class PaletteWidget;

namespace voxedit {

/**
 * @brief Voxel editing tools panel
 */
class VoxEditWindow: public ui::Window {
	friend class ::VoxEdit;
private:
	EditorScene* _scene;
	EditorScene* _sceneTop = nullptr;
	EditorScene* _sceneLeft = nullptr;
	EditorScene* _sceneFront = nullptr;
	VoxEdit* _voxedit;
	PaletteWidget* _paletteWidget;
	tb::TBWidget* _exportButton = nullptr;
	tb::TBWidget* _saveButton = nullptr;
	tb::TBWidget* _undoButton = nullptr;
	tb::TBWidget* _redoButton = nullptr;

	std::string _voxelizeFile;
	std::string _loadFile;

	tb::TBCheckBox *_showGrid = nullptr;
	tb::TBCheckBox *_showAABB = nullptr;
	tb::TBCheckBox *_showAxis = nullptr;
	tb::TBCheckBox *_freeLook = nullptr;

	std::string _exportFilter;
	bool _fourViewAvailable = false;

	void setQuadViewport(bool active);

	bool handleClickEvent(const tb::TBWidgetEvent &ev);
	bool handleChangeEvent(const tb::TBWidgetEvent &ev);
	void resetCameras();
	void quit();

	// commands
	void copy();
	void paste();
	void cut();
	void undo();
	void redo();
	void rotatex();
	void rotatey();
	void rotatez();
	void toggleviewport();
	void togglefreelook();
	void unselectall();
	bool voxelize(std::string_view file);
	bool save(std::string_view file);
	bool load(std::string_view file);
	bool exportFile(std::string_view file);
	bool createNew(bool force);
	void select(const glm::ivec3& pos);

public:
	VoxEditWindow(VoxEdit* tool);
	bool init();

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnProcess() override;
	void OnDie() override;
};

}
