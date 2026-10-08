/**
 * @file
 */

#include "BrushPanelShape.h"
#include "BrushPanelWidgets.h"
#include "Toolbar.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
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
			ImGui::BeginDisabled(modifier.shapeBrush().anyStrokeMode() && modifier.shapeBrush().radius() == 0 &&
							 (ShapeType)i != ShapeType::Box);
			toolbar.button(ShapeTypeIcons[i], cmd.c_str(), !active);
			ImGui::EndDisabled();
		}
	}

	if (currentSelectedShapeType == ShapeType::Circle || currentSelectedShapeType == ShapeType::Torus) {
		int thickness = modifier.shapeBrush().thickness();
		if (ImGui::InputIntWithButtons(_("Thickness"), &thickness)) {
			modifier.shapeBrush().setThickness(thickness);
		}
	}
}

void BrushPanelShape::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	ShapeBrush &brush = modifier.shapeBrush();
	// Remember the options of the active drawing method before switching methods.
	if (brush.anyStrokeMode()) {
		_strokeNoOverlap = brush.strokeNoOverlap();
	} else {
		_startFromCenter = brush.centerMode();
	}
	ImGui::TextUnformatted(_("Draw with"));
	if (ImGui::RadioButton(_("Stroke"), brush.anyStrokeMode())) {
		command::executeCommands(_strokeNoOverlap ? "setshapebrushstrokenooverlap" : "setshapebrushstroke", &listener);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(_("Drag shape"), !brush.anyStrokeMode())) {
		command::executeCommands(_startFromCenter ? "setshapebrushcenter" : "setshapebrushbox", &listener);
	}

	ImGui::SeparatorText(_("Shape"));
	addShapes(ctx, listener);
	if (brush.anyStrokeMode()) {
		int radius = brush.radius();
		ImGui::TextUnformatted(_("Brush radius"));
		ImGui::SetNextItemWidth(-1);
		if (ImGui::SliderIntWithButtons("##radius", &radius, 0, glm::max(32, radius), false)) {
			brush.setRadius(radius);
		}
		ImGui::TextWrappedUnformatted(_("Hold and drag to draw with the selected shape."));
		if (brush.radius() == 0) {
			ImGui::TextWrappedUnformatted(_("Radius 0 draws a single cube. Increase the radius to use other shapes."));
			if (ImGui::Button(_("Increase radius"))) {
				brush.setRadius(2);
			}
		}
	} else {
		ImGui::TextUnformatted(_("Start from"));
		ImGui::CommandRadioButton(_("Corner"), "setshapebrushbox", brush.boxMode(), &listener);
		ImGui::SameLine();
		ImGui::CommandRadioButton(_("Center"), "setshapebrushcenter", brush.centerMode(), &listener);
		ImGui::TextWrappedUnformatted(_("Drag to define the shape's size."));
	}

	ImGui::SeparatorText(_("Mirror"));
	brushpanel::addMirrorPlanes(listener, brush);
	if (brush.anyStrokeMode() && ImGui::CollapsingHeader(_("Advanced"))) {
		bool noOverlap = brush.strokeNoOverlap();
		if (ImGui::Checkbox(_("Apply once per voxel per stroke"), &noOverlap)) {
			command::executeCommands(noOverlap ? "setshapebrushstrokenooverlap" : "setshapebrushstroke", &listener);
		}
	}
}

} // namespace voxedit
