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
#include "core/collection/List.h"
#include "math/Axis.h"
#include "voxelgenerator/TreeContext.h"

class Viewport;
class PaletteWidget;
class LayerWidget;

namespace ui {
namespace turbobadger {
class UIApp;
}
}

namespace voxedit {

/**
 * @brief Voxel editing tools panel
 */
class VoxEditWindow: public ui::turbobadger::Window {
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

	// lsystem related
	tb::TBEditField *_lsystemAxiom = nullptr;
	tb::TBEditField *_lsystemRules = nullptr;
	tb::TBInlineSelectDouble *_lsystemAngle = nullptr;
	tb::TBInlineSelectDouble *_lsystemLength = nullptr;
	tb::TBInlineSelectDouble *_lsystemWidth = nullptr;
	tb::TBInlineSelectDouble *_lsystemWidthIncrement = nullptr;
	tb::TBInlineSelect *_lsystemIterations = nullptr;
	tb::TBInlineSelectDouble *_lsystemLeavesRadius = nullptr;
	tb::TBWidget *_lsystemSection = nullptr;

	// noise related
	tb::TBInlineSelect* _octaves;
	tb::TBInlineSelectDouble* _frequency;
	tb::TBInlineSelectDouble* _lacunarity;
	tb::TBInlineSelectDouble* _gain;
	tb::TBWidget *_noiseSection = nullptr;

	// tree related
	enum TreeParameterWidgetType {
		Int, Float
	};
	struct TreeWidget {
		size_t ctxOffset = 0u; // offset in voxelgenerator::TreeContext
		TreeParameterWidgetType type = TreeParameterWidgetType::Int;
		voxelgenerator::TreeType treeType = voxelgenerator::TreeType::Max;
		tb::TBWidget* widget = nullptr;
	};
	core::List<TreeWidget> _treeWidgets;
	tb::TBSelectDropdown *_treeType = nullptr;
	tb::TBCheckBox *_treeAutoGenerateOnChange = nullptr;
	tb::TBWidget *_treeSection = nullptr;
	voxelgenerator::TreeContext _treeGeneratorContext;
	void switchTreeType(voxelgenerator::TreeType treeType);
	tb::TBWidget* createTreeParameterWidget(TreeParameterWidgetType type, tb::TBLayout* parent, const char *id, const char *name);

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

	bool handleEvent(const tb::TBWidgetEvent &ev);

	bool handleClickEvent(const tb::TBWidgetEvent &ev);
	bool handleChangeEvent(const tb::TBWidgetEvent &ev);
	void quit();

	void updateStatusBar();

	void afterLoad(const core::String& file);
public:
	VoxEditWindow(ui::turbobadger::UIApp* tool);
	~VoxEditWindow();
	bool init();

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
	void resetCamera();

	bool isLayerWidgetDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void update();
	bool isSceneHovered() const;

	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onProcess() override;
	void onDie() override;
};

}
