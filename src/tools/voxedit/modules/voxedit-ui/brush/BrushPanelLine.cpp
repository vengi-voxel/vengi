/**
 * @file
 */

#include "BrushPanelLine.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/LineBrush.h"

namespace voxedit {

void BrushPanelLine::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	ImGui::TextWrappedUnformatted(_("Draws a line from the reference position to the current cursor position"));
	Modifier &modifier = ctx.sceneMgr->modifier();
	LineBrush &brush = modifier.lineBrush();
	bool continuous = brush.continuous();
	if (ImGui::Checkbox(_("Continuous"), &continuous)) {
		brush.setContinuous(continuous);
	}
	ImGui::TooltipCommand("togglelinebrushcontinuous");
	bool bezier = brush.bezier();
	if (ImGui::Checkbox(_("Bezier"), &bezier)) {
		if (!bezier && brush.hasPendingChanges()) {
			modifier.brushApply();
		}
		brush.setBezier(bezier);
	}
	ImGui::TooltipCommand("togglelinebrushbezier");
	ImGui::TooltipTextUnformatted(_("First click locks the segment end, the gizmo edits its control point, and pending "
									"segments commit when you apply or leave the brush"));

	int thickness = brush.thickness();
	if (ImGui::InputInt(_("Thickness"), &thickness)) {
		brush.setThickness(thickness);
	}

	if (!bezier) {
		int sag = brush.sag();
		if (ImGui::InputInt(_("Sag"), &sag)) {
			brush.setSag(sag);
		}
		ImGui::TooltipTextUnformatted(_("Downward sag in voxels for rope/cable effect"));
	}

	if (bezier && brush.pendingSegmentCount() > 0) {
		ImGui::SeparatorText(_("Pending segments"));
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
										   ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("##beziersegments", 2, tableFlags)) {
			const uint32_t fixedCol = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize;
			ImGui::TableSetupColumn(_("Segment"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##segdel", fixedCol);

			int removeIdx = -1;
			for (int i = 0; i < brush.pendingSegmentCount(); ++i) {
				if (brush.segment(i) == nullptr) {
					continue;
				}
				ImGui::PushID(i);
				const bool selected = brush.selectedSegment() == i;

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				const core::String label = core::String::format(_("Segment %i"), i + 1);
				if (ImGui::Selectable(label.c_str(), selected,
									  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
					brush.selectSegment(i);
				}

				ImGui::TableNextColumn();
				if (ImGui::IconButton(ICON_LC_TRASH_2, "")) {
					removeIdx = i;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();

			if (removeIdx != -1) {
				brush.removeSegment(removeIdx);
			}
		}
		ImGui::TooltipTextUnformatted(_("Select a pending segment to edit its control point with the gizmo"));
	}

	ImGui::TextUnformatted(_("Stipple pattern"));
	ImGui::SameLine();
	ImGui::BeginGroup();
	if (ImGui::StipplePattern("##linestipple", brush.stipplePattern())) {
		brush.markDirty();
	}
	ImGui::EndGroup();
	ImGui::TooltipTextUnformatted(
		_("Click cells to toggle: dash places a voxel, gap skips. Pattern length <= 1 disables stippling"));
}

} // namespace voxedit
