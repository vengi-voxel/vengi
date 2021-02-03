/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "math/Axis.h"
#include "video/WindowedApp.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/AbstractMainWindow.h"
#include "voxedit-util/SceneSettings.h"
#include "voxedit-util/layer/Layer.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

class Viewport;

class VoxEditWindow : public AbstractMainWindow {
private:
	using Super = AbstractMainWindow;

	core::DynamicArray<core::String> _scripts;

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

	struct NoiseData {
		int octaves = 4;
		float frequency = 0.01f;
		float lacunarity = 2.0f;
		float gain = 0.5f;
	};
	NoiseData _noiseData;

	struct LSystemData {
		core::String axiom = "F";
		core::String rulesStr = R"(
			{
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

	void menuBar();
	void palette();
	void tools();
	void layers();
	void statusBar();

	void leftWidget();
	void mainWidget();
	void rightWidget();

	void afterLoad(const core::String &file);

	void updateSettings();
	void registerPopups();

	void executeCommand(const char *command);

	void addLayerItem(int layerId, const voxedit::Layer &layer);

	bool actionButton(const char *title, const char *command);
	bool actionMenuItem(const char *title, const char *command, bool enabled = true);
	bool modifierRadioButton(const char *title, ModifierType type);
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	bool saveImage(const char *file) override;

	void switchTreeType(voxelgenerator::TreeType treeType);
	void treePanel();
	void lsystemPanel();
	void noisePanel();
	void scriptPanel();
	void modifierPanel();
	void positionsPanel();

public:
	VoxEditWindow(video::WindowedApp *app);
	virtual ~VoxEditWindow();
	bool init();
	void shutdown();

	// commands
	void toggleViewport();
	void toggleAnimation();
	bool save(const core::String &file);
	bool load(const core::String &file);
	bool loadAnimationEntity(const core::String &file);
	bool createNew(bool force);

	bool isLayerWidgetDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void resetCamera() override;
	void update();
	bool isSceneHovered() const;
};

} // namespace voxedit
