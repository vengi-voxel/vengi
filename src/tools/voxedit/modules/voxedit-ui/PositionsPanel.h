/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

class PositionsPanel {
private:
	bool _lastChanged = false;
	bool _localSpace = false;
	core::VarPtr _gizmoOperations;
	core::VarPtr _regionSizes;
	core::VarPtr _showGizmo;

	void modelView(command::CommandExecutionListener &listener);
	void keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	void keyFrameActionsAndOptions(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   scenegraph::FrameIndex frameIdx, scenegraph::KeyFrameIndex keyFrameIdx);
	void sceneView(command::CommandExecutionListener &listener);

public:
	bool init();
	void shutdown();
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
