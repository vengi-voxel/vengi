/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"
#include "core/String.h"
#include "math/Axis.h"
#include "editorscene/Action.h"
#include "voxedit-util/SelectType.h"

class EditorScene;
class VoxEdit;
class PaletteWidget;

namespace voxedit {

/**
 * @brief Voxel editing tools panel
 */
class VoxEditWindow: public ui::turbobadger::Window {
	friend class ::VoxEdit;
private:
	using Super = ui::turbobadger::Window;
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

	tb::TBEditField* _cursorX = nullptr;
	tb::TBEditField* _cursorY = nullptr;
	tb::TBEditField* _cursorZ = nullptr;

	tb::TBCheckBox* _lockedX = nullptr;
	tb::TBCheckBox* _lockedY = nullptr;
	tb::TBCheckBox* _lockedZ = nullptr;

	tb::TBRadioButton* _mirrorX = nullptr;
	tb::TBRadioButton* _mirrorY = nullptr;
	tb::TBRadioButton* _mirrorZ = nullptr;

	std::string _voxelizeFile;
	std::string _loadFile;

	tb::TBGenericStringItemSource _treeItems;
	tb::TBGenericStringItemSource _fileItems;
	tb::TBGenericStringItemSource _structureItems;
	tb::TBGenericStringItemSource _plantItems;
	tb::TBGenericStringItemSource _buildingItems;

	tb::TBCheckBox *_showGrid = nullptr;
	tb::TBCheckBox *_showAABB = nullptr;
	tb::TBCheckBox *_showAxis = nullptr;
	tb::TBCheckBox *_showLockAxis = nullptr;
	tb::TBCheckBox *_freeLook = nullptr;

	std::string _exportFilter;
	std::string _importFilter;
	bool _fourViewAvailable = false;
	bool _lockedDirty = false;
	bool _mirrorDirty = false;

	glm::ivec3 _lastCursorPos;

	bool handleEvent(const tb::TBWidgetEvent &ev);

	enum class ModifierMode {
		None,
		Rotate,
		Scale,
		Move,
		Lock,
		Mirror
	};
	ModifierMode _mode = ModifierMode::None;
	math::Axis _axis = math::Axis::None;
	static constexpr int MODENUMBERBUFSIZE = 64;
	char _modeNumberBuf[MODENUMBERBUFSIZE];
	long _lastModePress = -1l;
	void executeMode();

	void setQuadViewport(bool active);
	void setSelectionType(SelectType type);
	void setAction(Action action);

	bool handleClickEvent(const tb::TBWidgetEvent &ev);
	bool handleChangeEvent(const tb::TBWidgetEvent &ev);
	void resetcamera();
	void quit();

	// commands
	void place();
	void remove();
	void copy();
	void paste();
	void cut();
	void undo();
	void redo();
	void rotatex();
	void rotatey();
	void rotatez();
	void rotate(int x, int y, int z);
	void scalecursorx();
	void scalecursory();
	void scalecursorz();
	bool resample(int factor);
	void scaleCursor(float x, float y, float z);
	void movex();
	void movey();
	void movez();
	void move(int x, int y, int z);
	void crop();
	// fill at cursor position
	void fill();
	void fill(const glm::ivec3& pos);
	void extend(const glm::ivec3& size = glm::ivec3(1));
	void scaleHalf();
	void setCursorPosition(int x, int y, int z, bool relative = false);
	void toggleviewport();
	void togglefreelook();
	void movemode();
	void bezier(const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control);
	void scalemode();
	void rotatemode();
	void togglelockaxis();
	void togglemirroraxis();
	void setReferencePosition(int x = 0, int y = 0, int z = 0);
	void setreferencepositiontocursor();
	void unselectall();
	bool importMesh(const std::string& file);
	bool importHeightmap(const std::string& file);
	bool save(const std::string& file);
	bool load(const std::string& file);
	bool prefab(const std::string& file);
	bool exportFile(const std::string& file);
	bool createNew(bool force);
	void selectCursor();
	void select(const glm::ivec3& pos);

public:
	VoxEditWindow(VoxEdit* tool);
	~VoxEditWindow();
	bool init();

	void update();
	bool isFocused() const;
	bool isHovered() const;
	bool isActive() const;

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
	void OnProcess() override;
	void OnDie() override;
};

inline bool VoxEditWindow::isActive() const {
	return isFocused() || isHovered();
}

inline void VoxEditWindow::rotatex() {
	rotate(90, 0, 0);
}

inline void VoxEditWindow::rotatey() {
	rotate(0, 90, 0);
}

inline void VoxEditWindow::rotatez() {
	rotate(0, 0, 90);
}

inline void VoxEditWindow::movex() {
	move(1, 0, 0);
}

inline void VoxEditWindow::movey() {
	move(0, 1, 0);
}

inline void VoxEditWindow::movez() {
	move(0, 0, 1);
}

inline void VoxEditWindow::scalecursorx() {
	scaleCursor(2.0f, 1.0f, 1.0f);
}

inline void VoxEditWindow::scalecursory() {
	scaleCursor(1.0f, 2.0f, 1.0f);
}

inline void VoxEditWindow::scalecursorz() {
	scaleCursor(1.0f, 1.0f, 2.0f);
}


}
