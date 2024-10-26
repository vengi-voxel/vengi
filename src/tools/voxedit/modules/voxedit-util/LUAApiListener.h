/**
 * @file
 */

#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphListener.h"

namespace voxedit {

/**
 * @brief Interceptor for lua api calls that are directly modifying the scenegraph instance without the SceneManager
 * being able to record the memento states properly.
 * @note Only used during the script api calls - unregistered once the script execution is done.
 */
class LUAApiListener : public scenegraph::SceneGraphListener {
private:
	memento::MementoHandler &_mementoHandler;
	const scenegraph::SceneGraph &_sceneGraph;

public:
	LUAApiListener(memento::MementoHandler &mementoHandler, const scenegraph::SceneGraph &sceneGraph)
		: _mementoHandler(mementoHandler), _sceneGraph(sceneGraph) {
	}

	void onNodeAdded(int nodeId) override {
		_mementoHandler.markNodeAdded(_sceneGraph, _sceneGraph.node(nodeId));
	}

	void onNodeRemove(int nodeId) override {
		_mementoHandler.markNodeRemove(_sceneGraph, _sceneGraph.node(nodeId));
	}

	void onAnimationAdded(const core::String &name) override {
		_mementoHandler.markAnimationAdded(_sceneGraph, name);
	}

	void onAnimationRemoved(const core::String &name) override {
		_mementoHandler.markAnimationRemoved(_sceneGraph, name);
	}

	void onNodeChangedParent(int nodeId) override {
		_mementoHandler.markNodeMoved(_sceneGraph, _sceneGraph.node(nodeId));
	}

	void onNodesAligned() override {
		// TODO: MEMENTO: record all nodes
	}
};

} // namespace voxedit
