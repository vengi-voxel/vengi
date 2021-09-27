/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "math/Axis.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/layer/Layer.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxelgenerator/LUAGenerator.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

class Viewport;

class VoxEditWindow {
private:
	int _currentScript = -1;
	core::DynamicArray<voxelgenerator::LUAScript> _scripts;
	core::DynamicArray<voxelgenerator::LUAParameterDescription> _scriptParameterDescription;
	core::DynamicArray<core::String> _scriptParameters;

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
	bool _scriptEditor = false;

	struct NoiseData {
		int octaves = 4;
		float frequency = 0.01f;
		float lacunarity = 2.0f;
		float gain = 0.5f;
	};
	NoiseData _noiseData;

	struct LSystemData {
		core::String axiom = "F";
		core::String rulesStr = R"({
	F
	(67)F+[!+F-F-F(37)L]-[!-F+F+F(142)L]>[!F<F<F(128)L]<[!<F>F>F(123)L]
})";
		float angle = 22.5f;
		float length = 12.0f;
		float width = 4.0f;
		float widthIncrement = 1.5f;
		int iterations = 2;
		float leavesRadius = 8.0f;
	};
	LSystemData _lsystemData;
	core::String _currentSelectedPalette;
	core::DynamicArray<core::String> _availablePalettes;
	ui::imgui::IMGUIApp* _app;
	core::VarPtr _lastOpenedFile;

	LayerSettings _layerSettings;

	core::String _activeScript;
	core::String _editScript;
	core::String _loadFile;
	core::String _lastExecutedCommand;

	voxelgenerator::TreeContext _treeGeneratorContext;

	void reloadAvailablePalettes();

	void menuBar();
	void palette();
	void tools();
	void layers();
	void statusBar();

	void leftWidget();
	void mainWidget();
	void rightWidget();

	void dialog(const char *icon, const char *text);

	void afterLoad(const core::String &file);

	void updateSettings();
	void registerPopups();

	void executeCommand(const char *command);

	void addLayerItem(int layerId, const voxedit::Layer &layer);

	bool actionButton(const char *title, const char *command, const char *tooltip = nullptr, float width = 0.0f);
	void urlButton(const char *title, const char *url);
	bool actionMenuItem(const char *title, const char *command, bool enabled = true);
	void urlItem(const char *title, const char *url);
	bool modifierRadioButton(const char *title, ModifierType type);
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	bool saveImage(const char *file);
	void reloadScriptParameters(const core::String& script);

	void switchTreeType(voxelgenerator::TreeType treeType);
	void treePanel();
	void lsystemPanel();
	void noisePanel();
	void scriptPanel();
	void modifierPanel();
	void positionsPanel();

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

} // namespace voxedit
