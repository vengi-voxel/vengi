/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"

namespace voxel {
class SceneGraph;
class SceneGraphNode;
}

namespace voxedit {

struct LayerSettings;

class LayerPanel {
private:
	void addLayerItem(const voxel::SceneGraph& sceneGraph, const voxel::SceneGraphNode &node, command::CommandExecutionListener &listener);
	core::VarPtr _animationSpeedVar;

public:
	void update(const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener);
};

}
