/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "math/Axis.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"
#include "ui/imgui/TextEditor.h"
#include "voxedit-ui/AnimationPanel.h"
#include "voxedit-ui/CursorPanel.h"
#include "voxedit-ui/LSystemPanel.h"
#include "voxedit-ui/LayerPanel.h"
#include "voxedit-ui/ModifierPanel.h"
#include "voxedit-ui/NoisePanel.h"
#include "voxedit-ui/PalettePanel.h"
#include "voxedit-ui/ScriptPanel.h"
#include "voxedit-ui/ToolsPanel.h"
#include "voxedit-ui/TreePanel.h"
#include "voxedit-util/layer/Layer.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

class Viewport;

class VoxEditWindow {
private:
	struct LastExecutedCommand : public command::CommandExecutionListener {
		core::String command;
		void operator()(const core::String& cmd, const core::DynamicArray<core::String> &args) override {
			command = cmd;
		}
	};

	core::VarPtr _showAxisVar;
	core::VarPtr _showGridVar;
	core::VarPtr _modelSpaceVar;
	core::VarPtr _showLockedAxisVar;
	core::VarPtr _showAabbVar;
	core::VarPtr _renderShadowVar;
	core::VarPtr _animationSpeedVar;
	core::VarPtr _gridSizeVar;

	Viewport* _scene = nullptr;
	Viewport* _sceneTop = nullptr;
	Viewport* _sceneLeft = nullptr;
	Viewport* _sceneFront = nullptr;
	Viewport* _sceneAnimation = nullptr;

	bool _popupUnsaved = false;
	bool _popupNewScene = false;
	bool _popupFailedToSave = false;
	bool _popupSceneSettings = false;

	ui::imgui::IMGUIApp* _app;
	core::VarPtr _lastOpenedFile;

	LayerSettings _layerSettings;

	core::String _loadFile;

	LastExecutedCommand _lastExecutedCommand;
	NoisePanel _noisePanel;
	LSystemPanel _lsystemPanel;
	ScriptPanel _scriptPanel;
	TreePanel _treePanel;
	LayerPanel _layerPanel;
	AnimationPanel _animationPanel;
	ModifierPanel _modifierPanel;
	CursorPanel _cursorPanel;
	ToolsPanel _toolsPanel;
	PalettePanel _palettePanel;

	void menuBar();
	void statusBar();

	void leftWidget();
	void mainWidget();
	void rightWidget();

	void dialog(const char *icon, const char *text);

	void afterLoad(const core::String &file);

	void updateSettings();
	void registerPopups();

	bool actionButton(const char *title, const char *command, const char *tooltip = nullptr, float width = 0.0f);
	bool actionMenuItem(const char *title, const char *command, bool enabled = true);
	bool modifierRadioButton(const char *title, ModifierType type);
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	bool saveImage(const char *file);

public:
	VoxEditWindow(ui::imgui::IMGUIApp *app);
	virtual ~VoxEditWindow();
	bool init();
	void shutdown();

	// commands
	bool save(const core::String &file);
	bool load(const core::String &file);
	bool loadAnimationEntity(const core::String &file);
	bool createNew(bool force);

	bool isLayerWidgetDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void resetCamera();
	void update();
	bool isSceneHovered() const;

	bool importHeightmap(const core::String& file);
	bool importAsPlane(const core::String& file);
	bool importPalette(const core::String& file);

	bool saveScreenshot(const core::String& file);
	bool prefab(const core::String& file);
};

}
