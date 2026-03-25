/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/SharedPtr.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include <glm/vec3.hpp>

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
class AABBBrush;
class Brush;

class BrushPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	core::String _stamp;
	core::String _lastFont;
	int _stampPaletteIndex = 0;
	SceneManagerPtr _sceneMgr;
	video::TexturePoolPtr _texturePool;
	core::VarPtr _renderNormals;
	core::VarPtr _viewMode;

	// Cached transform UI values - synced to brush before execute
	glm::ivec3 _transformMoveOffset{0};
	glm::ivec3 _transformShearOffset{0};
	glm::vec3 _transformScale{1.0f};
	glm::vec3 _transformRotation{0.0f};
	bool _transformDirty = false;
	bool _transformUniformScale = true;

	void createPopups(command::CommandExecutionListener &listener);

	void addModifiers(command::CommandExecutionListener &listener);
	void brushSettings(command::CommandExecutionListener &listener);

	void addShapes(command::CommandExecutionListener &listener);
	void addMirrorPlanes(command::CommandExecutionListener &listener, Brush &brush);
	bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener, Brush &brush);

	void stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette,
								   command::CommandExecutionListener &listener);
	void stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette, command::CommandExecutionListener &listener);
	void updateStampBrushPanel(command::CommandExecutionListener &listener);
	void updatePlaneBrushPanel(command::CommandExecutionListener &listener);
	void updateLineBrushPanel(command::CommandExecutionListener &listener);
	void updatePathBrushPanel(command::CommandExecutionListener &listener);
	void updateSelectBrushPanel(command::CommandExecutionListener &listener);
	void updateTextureBrushPanel(command::CommandExecutionListener &listener);
	void updateNormalBrushPanel(command::CommandExecutionListener &listener);
	void updateExtrudeBrushPanel(command::CommandExecutionListener &listener);
	void executeExtrudeBrush();
	void updateTransformBrushPanel(command::CommandExecutionListener &listener);
	void executeTransformBrush();
	void updateRulerBrushPanel(command::CommandExecutionListener &listener);
	void updateSculptBrushPanel(command::CommandExecutionListener &listener);
	void executeSculptBrush();
	void loadSkinFromFile(const core::String &filename);

	void addBrushClampingOption(Brush &brush);
	void aabbBrushOptions(command::CommandExecutionListener &listener, AABBBrush &brush);
	void aabbBrushModeOptions(AABBBrush &brush);
	void updateShapeBrushPanel(command::CommandExecutionListener &listener);
	void updatePaintBrushPanel(command::CommandExecutionListener &listener);
	void updateTextBrushPanel(command::CommandExecutionListener &listener);

public:
	BrushPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool)
		: Super(app, "brush"), _sceneMgr(sceneMgr), _texturePool(texturePool) {
	}
	void init();
	void update(const char *id, bool sceneMode, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
