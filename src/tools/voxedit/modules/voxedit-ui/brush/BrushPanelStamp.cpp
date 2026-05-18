/**
 * @file
 */

#include "BrushPanelStamp.h"
#include "../DragAndDropPayload.h"
#include "BrushPanelWidgets.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxel/Region.h"
#include "voxelui/DragAndDropPayload.h"

namespace voxedit {

void BrushPanelStamp::stampBrushUseSelection(BrushPanelContext &ctx, scenegraph::SceneGraphNode &node,
											 palette::Palette &palette, command::CommandExecutionListener &listener) {
	ui::ScopedStyle selectionStyle;
	ImGui::BeginDisabled(!ctx.sceneMgr->hasSelection(node.id()));
	ImGui::CommandButton(_("Use selection"), "stampbrushuseselection", listener);
	ImGui::EndDisabled();
}

void BrushPanelStamp::stampBrushOptions(BrushPanelContext &ctx, scenegraph::SceneGraphNode &node,
										palette::Palette &palette, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	StampBrush &brush = modifier.stampBrush();
	brushpanel::addMirrorPlanes(listener, brush);
	ImGui::Separator();
	ImGui::InputTextWithHint(_("Model"), _("Select a model from the asset panel or scene graph panel"), &_stamp,
							 ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ModelPayload)) {
			const core::String &filename = *(core::String *)payload->Data;
			if (brush.load(filename)) {
				_stamp = filename;
			}
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload)) {
			int nodeId = *(int *)payload->Data;
			if (const scenegraph::SceneGraphNode *model = ctx.sceneMgr->sceneGraphModelNode(nodeId)) {
				brush.setVolume(*model->volume(), model->palette());
			}
		}
		ImGui::EndDragDropTarget();
	}

	bool center = brush.centerMode();
	if (ImGui::Checkbox(_("Center"), &center)) {
		command::executeCommands("togglestampbrushcenter", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcenter");
	bool continuous = brush.continuousMode();
	if (ImGui::Checkbox(_("Continuous"), &continuous)) {
		command::executeCommands("togglestampbrushcontinuous", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcontinuous");

	glm::ivec3 offset = brush.offset();
	if (ImGui::InputXYZ(_("Offset"), offset)) {
		brush.setOffset(offset);
	}

	brushpanel::addBrushClampingOption(brush);

	if (_stampPaletteIndex >= 0 && _stampPaletteIndex < palette.colorCount()) {
		const float size = ImGui::Height(1);
		const ImVec2 v1 = ImGui::GetCursorScreenPos();
		const ImVec2 v2(v1.x + size, v1.y + size);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		const uint32_t col = ImGui::GetColorU32(palette.color(palette.view().uiIndex(_stampPaletteIndex)));
		drawList->AddRectFilled(v1, v2, col);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
	}
	ImGui::InputInt("##colorstampbrush", &_stampPaletteIndex, 0, 0, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
			_stampPaletteIndex = *(const uint8_t *)payload->Data;
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Replace"))) {
		brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, _stampPaletteIndex), palette);
	}
	ImGui::TooltipTextUnformatted(_("Replace all voxels in the stamp with the selected color"));

	const float buttonWidth = ImGui::GetFontSize() * 4.0f;
	if (ImGui::CollapsingHeader(_("Rotate 90"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##rotateonaxis");
		ImGui::AxisCommandButton(math::Axis::X, _("X"), "stampbrushrotate x", ICON_LC_REPEAT, nullptr, buttonWidth,
								 &listener);
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Y, _("Y"), "stampbrushrotate y", ICON_LC_REPEAT, nullptr, buttonWidth,
								 &listener);
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Z, _("Z"), "stampbrushrotate z", ICON_LC_REPEAT, nullptr, buttonWidth,
								 &listener);
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(_("Crop stamp"))) {
		voxel::Region region = brush.volume()->region();
		glm::ivec3 size = region.getDimensionsInVoxels();
		if (ImGui::InputXYZ(_("Size"), size, nullptr, ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (glm::any(glm::greaterThan(size, region.getDimensionsInVoxels()))) {
				size = glm::min(size, region.getDimensionsInVoxels());
			}
			brush.setSize(size);
		}
	}
}

void BrushPanelStamp::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = ctx.sceneMgr->sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	palette::Palette &palette = node.palette();

	Modifier &modifier = ctx.sceneMgr->modifier();
	if (!modifier.stampBrush().active()) {
		ImGui::TextWrappedUnformatted(_("Select a model from the asset panel or scene graph panel"));
		ImGui::BeginDisabled();
		stampBrushOptions(ctx, node, palette, listener);
		ImGui::EndDisabled();
	} else {
		stampBrushOptions(ctx, node, palette, listener);
	}

	stampBrushUseSelection(ctx, node, palette, listener);
	if (ImGui::Button(_("Convert palette"))) {
		modifier.stampBrush().convertToPalette(palette);
	}
}

} // namespace voxedit
