/**
 * @file
 */

#include "BrushPanelExtrude.h"
#include "Style.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/ExtrudeBrush.h"
#include "voxel/Face.h"

namespace voxedit {

void BrushPanelExtrude::executeExtrudeBrush(BrushPanelContext &ctx) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	if (!modifier.beginBrushFromPanel()) {
		return;
	}
	ctx.sceneMgr->nodeForeachGroup([&](int nodeId) {
		if (scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			auto callback = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
				ctx.sceneMgr->modified(nodeId, region, flags);
			};
			modifier.execute(ctx.sceneMgr->sceneGraph(), *node, callback);
		}
	});
	modifier.endBrush();
	modifier.extrudeBrush().markDirty();
}

void BrushPanelExtrude::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	ExtrudeBrush &brush = modifier.extrudeBrush();

	const int nodeId = ctx.sceneMgr->sceneGraph().activeNode();
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphModelNode(nodeId);

	// Fallback used before a node is loaded; overridden by actual node dimensions below.
	static constexpr int DefaultMaxExtrudeDepth = 250;
	int maxDepth = DefaultMaxExtrudeDepth;
	if (node) {
		const voxel::Region &r = node->region();
		maxDepth = glm::max(r.getWidthInVoxels(), glm::max(r.getHeightInVoxels(), r.getDepthInVoxels()));
	}

	const voxel::FaceNames extrudeFace = brush.face();

	const glm::vec4 &warningTextColor = style::color(style::StyleColor::ColorWarningText);
	if (extrudeFace == voxel::FaceNames::Max) {
		ImGui::TextColored(warningTextColor, "%s",
						   _("Click a voxel face in the viewport to set the extrusion direction"));
		return;
	}

	ImGui::Text(_("Direction: %s"), voxel::faceNameString(extrudeFace));

	int depth = brush.depth();
	if (depth == 0 && !ctx.sceneMgr->hasSelection(nodeId)) {
		ImGui::TextColored(warningTextColor, "%s", _("No selection active - use the Select brush first"));
	}
	// All depth/offset changes are preview-only. The preview system creates a fresh
	// volume copy each frame and generate() writes to it. Commit happens on brush switch.
	ImGui::TextUnformatted(_("Depth"));
	if (ImGui::Button("-##extrude_depth")) {
		brush.setDepth(depth - 1);
	}
	ImGui::SameLine();
	if (ImGui::SliderInt("##extrude_depth_slider", &depth, -maxDepth, maxDepth)) {
		brush.setDepth(depth);
	}
	ImGui::SameLine();
	if (ImGui::Button("+##extrude_depth")) {
		brush.setDepth(depth + 1);
	}

	bool fillWalls = brush.fillWalls();
	if (ImGui::Checkbox(_("Fill walls"), &fillWalls)) {
		brush.setFillWalls(fillWalls);
	}

	// Lateral offset sliders - only shown when depth is non-zero.
	if (depth != 0) {
		static constexpr int NumAxes = 3;
		static constexpr const char *AxisLabels[] = {"X", "Y", "Z"};
		const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(extrudeFace));
		const int perp1 = (axisIdx + 1) % NumAxes;
		const int perp2 = (axisIdx + 2) % NumAxes;

		int offsetU = brush.offsetU();
		ImGui::Text(_("Offset %s"), AxisLabels[perp1]);
		if (ImGui::Button("-##extrude_offsetU")) {
			brush.setOffsetU(offsetU - 1);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##extrude_offsetU_slider", &offsetU, -maxDepth, maxDepth)) {
			brush.setOffsetU(offsetU);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##extrude_offsetU")) {
			brush.setOffsetU(offsetU + 1);
		}

		int offsetV = brush.offsetV();
		ImGui::Text(_("Offset %s"), AxisLabels[perp2]);
		if (ImGui::Button("-##extrude_offsetV")) {
			brush.setOffsetV(offsetV - 1);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##extrude_offsetV_slider", &offsetV, -maxDepth, maxDepth)) {
			brush.setOffsetV(offsetV);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##extrude_offsetV")) {
			brush.setOffsetV(offsetV + 1);
		}
	}
}

} // namespace voxedit
