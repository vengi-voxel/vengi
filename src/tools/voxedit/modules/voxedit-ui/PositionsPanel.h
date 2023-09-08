/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

class PositionsPanel {
private:
	bool _lastChanged = false;
	bool _localSpace = false;
	void modelView(command::CommandExecutionListener &listener);
	void keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	void keyFrameActionsAndOptions(const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   scenegraph::FrameIndex frameIdx, scenegraph::KeyFrameIndex keyFrameIdx);
	void sceneView(command::CommandExecutionListener &listener);

public:
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
