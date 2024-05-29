/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"

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
	int _stampPaletteIndex = 0;
	SceneManagerPtr _sceneMgr;

	void addModifiers(command::CommandExecutionListener &listener);
	void brushSettings(command::CommandExecutionListener &listener);

	void addShapes(command::CommandExecutionListener &listener);
	void addMirrorPlanes(command::CommandExecutionListener &listener, AABBBrush &brush);
	bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener, AABBBrush &brush);

	void stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette,
								   command::CommandExecutionListener &listener);
	void stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette, command::CommandExecutionListener &listener);
	void updateStampBrushPanel(command::CommandExecutionListener &listener);
	void updatePlaneBrushPanel(command::CommandExecutionListener &listener);
	void updateLineBrushPanel(command::CommandExecutionListener &listener);
	void updatePathBrushPanel(command::CommandExecutionListener &listener);

	void addBrushClampingOption(Brush &brush);
	void aabbBrushOptions(command::CommandExecutionListener &listener, AABBBrush &brush);
	void aabbBrushModeOptions(AABBBrush &brush);
	void updateShapeBrushPanel(command::CommandExecutionListener &listener);
	void updatePaintBrushPanel(command::CommandExecutionListener &listener);
	void updateTextBrushPanel(command::CommandExecutionListener &listener);
public:
	BrushPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "brush"), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *title) override;
#endif
};

}
