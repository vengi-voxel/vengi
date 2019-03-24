/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "core/Common.h"
#include "voxedit-util/ModifierType.h"
#include "core/String.h"
#include "math/Axis.h"

class Viewport;
class VoxEdit;
class PaletteWidget;
class LayerWidget;

namespace voxedit {

/**
 * @brief Voxel editing tools panel
 */
class VoxEditWindow: public ui::turbobadger::Window {
	friend class ::VoxEdit;
private:
	using Super = ui::turbobadger::Window;
	Viewport* _scene;
	Viewport* _sceneTop = nullptr;
	Viewport* _sceneLeft = nullptr;
	Viewport* _sceneFront = nullptr;
	VoxEdit* _voxedit;
	PaletteWidget* _paletteWidget;
	LayerWidget* _layerWidget;
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

	tb::TBRadioButton* _mirrorNone = nullptr;
	tb::TBRadioButton* _mirrorX = nullptr;
	tb::TBRadioButton* _mirrorY = nullptr;
	tb::TBRadioButton* _mirrorZ = nullptr;

	tb::TBRadioButton* _placeModifier = nullptr;
	tb::TBRadioButton* _deleteModifier = nullptr;
	tb::TBRadioButton* _overrideModifier = nullptr;
	tb::TBRadioButton* _colorizeModifier = nullptr;
	tb::TBRadioButton* _extrudeModifier = nullptr;

	std::string _voxelizeFile;
	std::string _loadFile;

	tb::TBGenericStringItemSource _treeItems;
	tb::TBGenericStringItemSource _fileItems;
	tb::TBGenericStringItemSource _structureItems;
	tb::TBGenericStringItemSource _plantItems;
	tb::TBGenericStringItemSource _buildingItems;

	tb::TBInlineSelect *_voxelSize = nullptr;
	tb::TBCheckBox *_showGrid = nullptr;
	tb::TBCheckBox *_showAABB = nullptr;
	tb::TBCheckBox *_showAxis = nullptr;
	tb::TBCheckBox *_showLockAxis = nullptr;
	tb::TBCheckBox *_renderShadow = nullptr;

	std::string _exportFilter;
	std::string _importFilter;
	bool _fourViewAvailable = false;
	ModifierType _modBeforeMouse = ModifierType::None;
	core::VarPtr _lastOpenedFile;

	glm::ivec3 _lastCursorPos;

	bool handleEvent(const tb::TBWidgetEvent &ev);

	bool handleClickEvent(const tb::TBWidgetEvent &ev);
	bool handleChangeEvent(const tb::TBWidgetEvent &ev);
	void resetCamera();
	void quit();

	void afterLoad(const std::string& file);

	// commands
	void toggleviewport();
	bool importMesh(const std::string& file);
	bool importHeightmap(const std::string& file);
	bool save(const std::string& file);
	bool load(const std::string& file);
	bool saveScreenshot(const std::string& file);
	bool prefab(const std::string& file);
	bool exportFile(const std::string& file);
	bool createNew(bool force);
public:
	VoxEditWindow(VoxEdit* tool);
	~VoxEditWindow();
	bool init();

	void update();
	bool isFocused() const;
	bool isHovered() const;
	bool isActive() const;

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onProcess() override;
	void onDie() override;
};

inline bool VoxEditWindow::isActive() const {
	return isFocused() || isHovered();
}

}
