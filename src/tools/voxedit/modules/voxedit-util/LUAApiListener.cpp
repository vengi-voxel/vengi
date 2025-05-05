/**
 * @file
 */

#include "LUAApiListener.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

LUAApiListener::LUAApiListener(memento::MementoHandler &mementoHandler,
							   const scenegraph::SceneGraph &sceneGraph)
	: _mementoHandler(mementoHandler), _sceneGraph(sceneGraph) {
}

void LUAApiListener::onNodeAdded(int nodeId) {
	_mementoHandler.markNodeAdded(_sceneGraph, _sceneGraph.node(nodeId));
}

void LUAApiListener::onNodeRemove(int nodeId) {
	_mementoHandler.markNodeRemove(_sceneGraph, _sceneGraph.node(nodeId));
}

void LUAApiListener::onAnimationAdded(const core::String &name) {
	_mementoHandler.markAnimationAdded(_sceneGraph, name);
}

void LUAApiListener::onAnimationRemoved(const core::String &name) {
	_mementoHandler.markAnimationRemoved(_sceneGraph, name);
}

void LUAApiListener::onNodeChangedParent(int nodeId) {
	_mementoHandler.markNodeMoved(_sceneGraph, _sceneGraph.node(nodeId));
}

void LUAApiListener::onNodesAligned() {
	// TODO: MEMENTO: record all nodes
}

} // namespace voxedit
