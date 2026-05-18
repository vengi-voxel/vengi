/**
 * @file
 */

#include "BrushPanelWidgets.h"
#include "ScopedStyle.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/Brush.h"

namespace voxedit {
namespace brushpanel {

bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener,
						   Brush &brush) {
	core::String cmd = "mirroraxis" + brush.name().toLower() +
					   "brush"; // mirroraxisshapebrushx, mirroraxisshapebrushy, mirroraxisshapebrushz
	cmd += math::getCharForAxis(type);
	{
		ui::ScopedStyle style;
		ImGui::AxisStyleText(style, type);
		if (ImGui::RadioButton(title, brush.mirrorAxis() == type)) {
			command::executeCommands(cmd, &listener);
			return true;
		}
	}
	const core::String &help = command::help(cmd);
	if (!help.empty()) {
		ImGui::TooltipTextUnformatted(help.c_str());
	}
	return false;
}

void addMirrorPlanes(command::CommandExecutionListener &listener, Brush &brush) {
	ImGui::PushID("##mirrorplanes");
	mirrorAxisRadioButton(_("Disable mirror"), math::Axis::None, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("X"), math::Axis::X, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("Y"), math::Axis::Y, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("Z"), math::Axis::Z, listener, brush);
	ImGui::PopID();
}

void aabbBrushOptions(command::CommandExecutionListener &listener, AABBBrush &brush) {
	addMirrorPlanes(listener, brush);
	ImGui::Separator();

	const bool aabb = brush.aabbMode();
	core::String toggleAABBCmd = "set" + brush.name().toLower() + "brushaabb";
	ImGui::CommandRadioButton(_("Box"), toggleAABBCmd, aabb, &listener);

	const bool single = brush.singleMode();
	core::String toggleSingleCmd = "set" + brush.name().toLower() + "brushsingle";
	ImGui::CommandRadioButton(_("Single"), toggleSingleCmd, single, &listener);

	const bool singleMove = brush.singleModeMove();
	core::String toggleSingleMoveCmd = "set" + brush.name().toLower() + "brushsinglemove";
	ImGui::CommandRadioButton(_("Single Move"), toggleSingleMoveCmd, singleMove, &listener);

	const bool center = brush.centerMode();
	core::String toggleCenterCmd = "set" + brush.name().toLower() + "brushcenter";
	ImGui::CommandRadioButton(_("Center"), toggleCenterCmd, center, &listener);
}

// doing this after aabbBrushOptions() allows us to extend the radio buttons
void aabbBrushModeOptions(AABBBrush &brush) {
	if (brush.anySingleMode()) {
		int radius = brush.radius();
		if (ImGui::InputInt(_("Radius"), &radius)) {
			brush.setRadius(radius);
		}
		ImGui::TooltipTextUnformatted(_("Use a radius around the current voxel - 0 for spanning a region"));
	}
}

void addBrushClampingOption(Brush &brush) {
	bool clamping = brush.brushClamping();
	if (ImGui::Checkbox(_("Clamping"), &clamping)) {
		brush.setBrushClamping(clamping);
	}
	ImGui::TooltipTextUnformatted(_("Clamp the brush to the volume"));
}

} // namespace brushpanel
} // namespace voxedit
