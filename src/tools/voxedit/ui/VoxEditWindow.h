/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"
#include "layer/LayerWindow.h"
#include "settings/SceneSettingsWindow.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/collection/Array.h"
#include "math/Axis.h"
#include "voxelgenerator/TreeContext.h"

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
	Viewport* _scene = nullptr;
	Viewport* _sceneTop = nullptr;
	Viewport* _sceneLeft = nullptr;
	Viewport* _sceneFront = nullptr;
	Viewport* _sceneAnimation = nullptr;
	tb::TBLayout* _animationWidget = nullptr;
	PaletteWidget* _paletteWidget = nullptr;
	LayerWidget* _layerWidget = nullptr;
	tb::TBWidget* _saveButton = nullptr;
	tb::TBWidget* _saveAnimationButton = nullptr;
	tb::TBWidget* _undoButton = nullptr;
	tb::TBWidget* _redoButton = nullptr;

	tb::TBEditField* _cursorX = nullptr;
	tb::TBEditField* _cursorY = nullptr;
	tb::TBEditField* _cursorZ = nullptr;

	tb::TBEditField* _paletteIndex = nullptr;

	tb::TBCheckBox* _lockedX = nullptr;
	tb::TBCheckBox* _lockedY = nullptr;
	tb::TBCheckBox* _lockedZ = nullptr;

	tb::TBInlineSelect* _translateX = nullptr;
	tb::TBInlineSelect* _translateY = nullptr;
	tb::TBInlineSelect* _translateZ = nullptr;

	tb::TBRadioButton* _mirrorAxisNone = nullptr;
	tb::TBRadioButton* _mirrorAxisX = nullptr;
	tb::TBRadioButton* _mirrorAxisY = nullptr;
	tb::TBRadioButton* _mirrorAxisZ = nullptr;

	tb::TBRadioButton* _selectModifier = nullptr;
	tb::TBRadioButton* _placeModifier = nullptr;
	tb::TBRadioButton* _deleteModifier = nullptr;
	tb::TBRadioButton* _overrideModifier = nullptr;
	tb::TBRadioButton* _colorizeModifier = nullptr;

	tb::TBInlineSelect* _octaves;
	tb::TBInlineSelectDouble* _frequency;
	tb::TBInlineSelectDouble* _lacunarity;
	tb::TBInlineSelectDouble* _gain;

	tb::TBInlineSelect *_trunkHeight;
	tb::TBInlineSelect *_trunkWidth;
	tb::TBInlineSelect *_leavesWidth;
	tb::TBInlineSelect *_leavesHeight;
	tb::TBInlineSelect *_leavesDepth;
	tb::TBSelectDropdown *_treeType;
	tb::TBInlineSelect *_branchSize;
	tb::TBInlineSelectDouble *_branchFactor;
	tb::TBInlineSelect *_branches;
	tb::TBInlineSelect *_controlOffset;
	voxelgenerator::TreeContext _treeGeneratorContext;

	core::String _voxelizeFile;
	core::String _loadFile;

	core::String _lastExecutedCommand;

	tb::TBGenericStringItemSource _treeItems;
	tb::TBGenericStringItemSource _fileItems;
	tb::TBGenericStringItemSource _animationItems;

	tb::TBInlineSelect *_voxelSize = nullptr;
	tb::TBCheckBox *_showGrid = nullptr;
	tb::TBCheckBox *_showAABB = nullptr;
	tb::TBCheckBox *_showAxis = nullptr;
	tb::TBCheckBox *_showLockAxis = nullptr;
	tb::TBCheckBox *_renderShadow = nullptr;

	bool _fourViewAvailable = false;
	bool _animationViewAvailable = false;
	core::VarPtr _lastOpenedFile;

	glm::ivec3 _lastCursorPos;

	LayerSettings _layerSettings;
	SceneSettings _settings;

	core::Array<video::TexturePtr, 4> _backgrounds;

	bool handleEvent(const tb::TBWidgetEvent &ev);

	bool handleClickEvent(const tb::TBWidgetEvent &ev);
	bool handleChangeEvent(const tb::TBWidgetEvent &ev);
	void resetCamera();
	void quit();

	void updateStatusBar();

	void afterLoad(const core::String& file);

	// commands
	void toggleViewport();
	void toggleAnimation();
	bool importHeightmap(const core::String& file);
	bool importAsPlane(const core::String& file);
	bool importPalette(const core::String& file);
	bool save(const core::String& file);
	bool load(const core::String& file);
	bool loadAnimationEntity(const core::String& file);
	bool saveScreenshot(const core::String& file);
	bool prefab(const core::String& file);
	bool createNew(bool force);
public:
	VoxEditWindow(VoxEdit* tool);
	~VoxEditWindow();
	bool init();

	bool isLayerWidgetDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void update();
	bool isSceneHovered() const;

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onProcess() override;
	void onDie() override;
};

}
