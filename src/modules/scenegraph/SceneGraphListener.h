/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace scenegraph {

class SceneGraphListener {
public:
	virtual ~SceneGraphListener() = default;

	virtual void onNodeAdded(int nodeId) {
	}
	virtual void onNodeRemove(int nodeId) {
	}
	virtual void onAnimationAdded(const core::String &name) {
	}
	virtual void onAnimationRemoved(const core::String &name) {
	}
	virtual void onAnimationChanged(const core::String &name) {
	}
	virtual void onNodeChangedParent(int nodeId) {
	}
	virtual void onNodesAligned() {
	}
};

} // namespace scenegraph
