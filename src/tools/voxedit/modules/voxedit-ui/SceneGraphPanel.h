/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphNode.h"

namespace video {
class Camera;
}
namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
class SceneGraphNodeCamera;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

struct ModelNodeSettings;

class SceneGraphPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	core::VarPtr _animationSpeedVar;
	core::VarPtr _hideInactive;
	bool _hasFocus = false;
	bool _popupDragAndDrop = false;
	int _dragDropSourceNodeId = InvalidNodeId;
	int _dragDropTargetNodeId = InvalidNodeId;
	int _lastActivedNodeId = InvalidNodeId;
	SceneManagerPtr _sceneMgr;

	core::String _filterName;
	int _filterType = 0;
	bool isFiltered(const scenegraph::SceneGraphNode &node) const;

	void registerPopups();
	void detailView(scenegraph::SceneGraphNode &node);
	void recursiveAddNodes(video::Camera &camera, const scenegraph::SceneGraph &sceneGraph,
							  scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener,
							  int depth, int referencedNodeId);
	void contextMenu(video::Camera& camera, const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
public:
	SceneGraphPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "scenegraph"), _sceneMgr(sceneMgr) {
	}
	bool _popupNewModelNode = false;
	bool init();
	void update(video::Camera &camera, const char *id, ModelNodeSettings *modelNodeSettings,
				command::CommandExecutionListener &listener);
	bool hasFocus() const;
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

}
