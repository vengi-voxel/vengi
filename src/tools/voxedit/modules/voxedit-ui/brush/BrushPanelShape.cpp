/**
 * @file
 */

#include "BrushPanelShape.h"
#include "BrushPanelWidgets.h"
#include "Toolbar.h"
#include "command/CommandHandler.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ShapeType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"

namespace voxedit {

void BrushPanelShape::addShapes(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();

	const ShapeType currentSelectedShapeType = modifier.shapeBrush().shapeType();
	{
		ui::Toolbar toolbar("shapes", &listener);
		for (int i = 0; i < (int)ShapeType::Max; ++i) {
			const bool active = (ShapeType)i == currentSelectedShapeType;
			const core::String &cmd = core::String::format("shape%s", ShapeTypeCmdStr[i]);
			toolbar.button(ShapeTypeIcons[i], cmd.c_str(), !active);
		}
	}

	if (currentSelectedShapeType == ShapeType::Circle || currentSelectedShapeType == ShapeType::Torus) {
		int thickness = modifier.shapeBrush().thickness();
		if (ImGui::InputInt(_("Thickness"), &thickness)) {
			modifier.shapeBrush().setThickness(thickness);
		}
	}
}

void BrushPanelShape::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	ShapeBrush &brush = modifier.shapeBrush();
	addShapes(ctx, listener);
	brushpanel::aabbBrushOptions(listener, brush);
	brushpanel::aabbBrushModeOptions(brush);
}

} // namespace voxedit
