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

class NodeInspectorPanel : public ui::Panel {
private:
	using Super = ui ::Panel;
	bool _localSpace = false;
	core::VarPtr _regionSizes;
	SceneManagerPtr _sceneMgr;
	core::String _propertyKey;
	core::String _propertyValue;

	void modelView(command::CommandExecutionListener &listener);
	void keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	void keyFrameActionsAndOptions(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   scenegraph::FrameIndex frameIdx, scenegraph::KeyFrameIndex keyFrameIdx);
	void sceneView(command::CommandExecutionListener &listener);
	void detailView(scenegraph::SceneGraphNode &node);
	/**
	 * @return @c true if the property was handled with a special ui input widget - @c false if it should just be a
	 * normal text input field
	 */
	bool handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key, const core::String &value);

public:
	NodeInspectorPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "nodeinspector"), _sceneMgr(sceneMgr) {
	}
	bool init();
	void shutdown();
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *title) override;
#endif
};

} // namespace voxedit
