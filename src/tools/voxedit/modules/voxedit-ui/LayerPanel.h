/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"
#include "voxedit-util/layer/Layer.h"
#include "voxedit-util/layer/LayerSettings.h"

namespace voxedit {

class LayerPanel {
private:
	void addLayerItem(int layerId, const voxedit::Layer &layer, command::CommandExecutionListener &listener);
	core::VarPtr _animationSpeedVar;

public:
	void update(const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener);
};

}
