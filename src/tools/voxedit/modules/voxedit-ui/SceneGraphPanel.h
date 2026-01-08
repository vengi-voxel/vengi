/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"
#include "core/Var.h"
#include "core/collection/Set.h"
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
	bool _scrollToActiveNode = false;
	int _dragDropSourceNodeId = InvalidNodeId;
	int _dragDropTargetNodeId = InvalidNodeId;
	int _lastActivedNodeId = InvalidNodeId;
	SceneManagerPtr _sceneMgr;

	struct DisplayNode {
		DisplayNode(int _nodeId, int _depth, bool _hasChildren) :
			nodeId(_nodeId), depth(_depth), hasChildren(_hasChildren) {
		}
		int nodeId;
		int depth;
		bool hasChildren;
	};
	core::DynamicArray<DisplayNode> _displayNodes;
	core::Set<int> _collapsedNodes;

	core::String _filterName;
	int _filterType = 0;
	bool isFiltered(const scenegraph::SceneGraphNode &node) const;

	void registerPopups();
	void detailView(scenegraph::SceneGraphNode &node);
	void renderNode(video::Camera &camera, const scenegraph::SceneGraph &sceneGraph,
							  const DisplayNode &displayNode, command::CommandExecutionListener &listener,
							  int referencedNodeId);
	void rebuildDisplayList(const scenegraph::SceneGraph &sceneGraph, int nodeId, int depth);

	// remove nodes from collapsed list to ensure visibility of the given node
	void makeVisible(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);

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
