/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraphListener.h"

namespace memento {
class MementoHandler;
}

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

class SceneManager;

/**
 * @brief Interceptor for lua api calls that are directly modifying the scenegraph instance without the SceneManager
 * being able to record the memento states properly.
 * @note Only used during the script api calls - unregistered once the script execution is done.
 */
class LUAApiListener : public scenegraph::SceneGraphListener {
private:
	SceneManager *_sceneMgr;
	memento::MementoHandler &_mementoHandler;
	const scenegraph::SceneGraph &_sceneGraph;

public:
	LUAApiListener(SceneManager *sceneMgr, memento::MementoHandler &mementoHandler,
				   const scenegraph::SceneGraph &sceneGraph);

	void onNodeAdded(int nodeId) override;
	void onNodeRemove(int nodeId) override;
	void onAnimationAdded(const core::String &name) override;
	void onAnimationRemoved(const core::String &name) override;
	void onNodeChangedParent(int nodeId) override;
	void onNodesAligned() override;
};

} // namespace voxedit
