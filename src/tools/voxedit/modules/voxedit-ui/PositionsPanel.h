/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class PositionsPanel : public ui::Panel {
private:
	using Super = ui ::Panel;
	bool _lastChanged = false;
	bool _localSpace = false;
	core::VarPtr _gizmoOperations;
	core::VarPtr _regionSizes;
	core::VarPtr _showGizmo;
	SceneManagerPtr _sceneMgr;

	void modelView(command::CommandExecutionListener &listener);
	void keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	void keyFrameActionsAndOptions(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   scenegraph::FrameIndex frameIdx, scenegraph::KeyFrameIndex keyFrameIdx);
	void sceneView(command::CommandExecutionListener &listener);

public:
	PositionsPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "positions"), _sceneMgr(sceneMgr) {
	}
	bool init();
	void shutdown();
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *title) override;
#endif
};

} // namespace voxedit
