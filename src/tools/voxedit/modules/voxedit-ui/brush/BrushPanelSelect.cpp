/**
 * @file
 */

#include "BrushPanelSelect.h"
#include "Toolbar.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/LUASelectionMode.h"
#include "voxedit-util/modifier/brush/SelectBrush.h"
#include "voxel/Region.h"
#include "voxelui/LUAScriptParameters.h"

namespace voxedit {

void BrushPanelSelect::handleSelectBox3D(BrushPanelContext &ctx, int nodeId) {
	voxel::Region sel = ctx.sceneMgr->selectionCalculateRegion(nodeId);
	if (!sel.isValid()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	ImGui::SeparatorText(_("Selection bounds"));
	const voxel::Region &vol = node->region();
	const glm::ivec3 &vMins = vol.getLowerCorner();
	const glm::ivec3 &vMaxs = vol.getUpperCorner();
	glm::ivec3 mins = sel.getLowerCorner();
	glm::ivec3 maxs = sel.getUpperCorner();
	bool changed = false;

	// TODO: SELECTION: this should be a default imgui component already. The slider does afair als have the +/-
	// buttons, no? Slider with -/+ buttons for fine single-step adjustment
	auto selSlider = [&changed](const char *id, int *val, int lo, int hi) {
		ImGui::PushID(id);
		const float btnW = ImGui::GetFrameHeight();
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		if (ImGui::Button("-", ImVec2(btnW, 0))) {
			*val = glm::max(*val - 1, lo);
			changed = true;
		}
		ImGui::SameLine(0, spacing);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
		changed |= ImGui::SliderInt("", val, lo, hi);
		ImGui::SameLine(0, spacing);
		if (ImGui::Button("+", ImVec2(btnW, 0))) {
			*val = glm::min(*val + 1, hi);
			changed = true;
		}
		ImGui::PopID();
	};

	if (ImGui::BeginTable("##selbounds", 3, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("##axis", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn(_("Min"));
		ImGui::TableSetupColumn(_("Max"));
		ImGui::TableHeadersRow();

		ImGui::TableNextColumn();
		ImGui::AxisButtonX(ImVec2(ImGui::GetFrameHeight(), 0));
		ImGui::TableNextColumn();
		selSlider("##minx", &mins.x, vMins.x, vMaxs.x);
		ImGui::TableNextColumn();
		selSlider("##maxx", &maxs.x, vMins.x, vMaxs.x);

		ImGui::TableNextColumn();
		ImGui::AxisButtonY(ImVec2(ImGui::GetFrameHeight(), 0));
		ImGui::TableNextColumn();
		selSlider("##miny", &mins.y, vMins.y, vMaxs.y);
		ImGui::TableNextColumn();
		selSlider("##maxy", &maxs.y, vMins.y, vMaxs.y);

		ImGui::TableNextColumn();
		ImGui::AxisButtonZ(ImVec2(ImGui::GetFrameHeight(), 0));
		ImGui::TableNextColumn();
		selSlider("##minz", &mins.z, vMins.z, vMaxs.z);
		ImGui::TableNextColumn();
		selSlider("##maxz", &maxs.z, vMins.z, vMaxs.z);

		ImGui::EndTable();
	}

	if (changed) {
		// ensure min <= max after independent dragging
		const glm::ivec3 newMins = glm::min(mins, maxs);
		const glm::ivec3 newMaxs = glm::max(mins, maxs);
		ctx.sceneMgr->selectionSetBounds(node->id(), voxel::Region(newMins, newMaxs));
	}

	const glm::ivec3 size = sel.getDimensionsInVoxels();
	ImGui::Text(_("Size: %d x %d x %d"), size.x, size.y, size.z);
}

void BrushPanelSelect::handleSelectCircle(BrushPanelContext &ctx, int nodeId) {
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	select::Circle &circle = ctx.sceneMgr->modifier().selectBrush().circle();
	ImGui::SeparatorText(_("Circle selection"));
	const voxel::Region &vol = node->region();
	const glm::ivec3 &vMins = vol.getLowerCorner();
	const glm::ivec3 &vMaxs = vol.getUpperCorner();
	glm::ivec3 center = circle.center();
	int radiusU = circle.radiusU();
	int radiusV = circle.radiusV();
	bool changed = false;

	int uAxis;
	int vAxis;
	select::Circle::ellipseAxes(circle.face(), uAxis, vAxis);
	const char *axisNames[] = {"X", "Y", "Z"};

	auto ellipseSlider = [&changed](const char *id, int *val, int lo, int hi) {
		ImGui::PushID(id);
		const float btnW = ImGui::GetFrameHeight();
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		if (ImGui::Button("-", ImVec2(btnW, 0))) {
			*val = glm::max(*val - 1, lo);
			changed = true;
		}
		ImGui::SameLine(0, spacing);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
		changed |= ImGui::SliderInt("", val, lo, hi);
		ImGui::SameLine(0, spacing);
		if (ImGui::Button("+", ImVec2(btnW, 0))) {
			*val = glm::min(*val + 1, hi);
			changed = true;
		}
		ImGui::PopID();
	};

	ImGui::TextUnformatted(_("Center"));
	ellipseSlider("##cx", &center[uAxis], vMins[uAxis], vMaxs[uAxis]);
	ellipseSlider("##cy", &center[vAxis], vMins[vAxis], vMaxs[vAxis]);

	// Max radius is half the volume dimension on each axis
	const int maxRadiusU = (vMaxs[uAxis] - vMins[uAxis]) / 2;
	const int maxRadiusV = (vMaxs[vAxis] - vMins[vAxis]) / 2;
	ImGui::Text(_("Radius %s"), axisNames[uAxis]);
	ellipseSlider("##ru", &radiusU, 0, maxRadiusU);
	ImGui::Text(_("Radius %s"), axisNames[vAxis]);
	ellipseSlider("##rv", &radiusV, 0, maxRadiusV);

	const int faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(circle.face()));
	const int maxDepth = (vMaxs[faceAxisIdx] - vMins[faceAxisIdx]) / 2;
	int depth = circle.depth();
	ImGui::Text(_("Depth %s"), axisNames[faceAxisIdx]);
	ImGui::TooltipTextUnformatted(
		_("How far from the center the selection extends along the face-normal axis (0 = single layer)"));
	ellipseSlider("##depth", &depth, 1, maxDepth);

	bool is3D = circle.is3D();
	if (ImGui::Checkbox(_("3D ellipsoid"), &is3D)) {
		circle.set3D(is3D);
		changed = true;
	}
	ImGui::TooltipTextUnformatted(
		_("Select voxels in a 3D ellipsoid shape behind the clicked surface instead of a 2D ellipse with depth"));

	if (changed) {
		circle.setCenter(center);
		circle.setRadiusU(radiusU);
		circle.setRadiusV(radiusV);
		circle.setDepth(depth);
		ctx.sceneMgr->selectionSetEllipse(nodeId);
	}
}

void BrushPanelSelect::handleSelectPaint(BrushPanelContext &ctx, int nodeId) {
	SelectBrush &brush = ctx.sceneMgr->modifier().selectBrush();
	ImGui::SeparatorText(_("Paint selection"));
	static constexpr int MaxPaintRadius = 32;
	int rad = brush.radius();
	const float btnW = ImGui::GetFrameHeight();
	const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	ImGui::TextUnformatted(_("Radius"));
	ImGui::TooltipTextUnformatted(_("Brush radius for paint selection. Hold mouse and drag to select voxels."));
	ImGui::PushID("paintradius");
	if (ImGui::Button("-", ImVec2(btnW, 0))) {
		rad = glm::max(rad - 1, 0);
		brush.setRadius(rad);
	}
	ImGui::SameLine(0, spacing);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
	if (ImGui::SliderInt("##paintradius", &rad, 0, MaxPaintRadius)) {
		brush.setRadius(rad);
	}
	ImGui::SameLine(0, spacing);
	if (ImGui::Button("+", ImVec2(btnW, 0))) {
		rad = glm::min(rad + 1, MaxPaintRadius);
		brush.setRadius(rad);
	}
	ImGui::PopID();
	bool growRegion = brush.paint().growRegion();
	if (ImGui::Checkbox(_("Grow region"), &growRegion)) {
		brush.paint().setGrowRegion(growRegion);
	}
	ImGui::TooltipTextUnformatted(
		_("Only select voxels adjacent to already-selected voxels. Useful for expanding an existing selection."));
}

void BrushPanelSelect::handleSelectFuzzyColor(BrushPanelContext &ctx) {
	SelectBrush &brush = ctx.sceneMgr->modifier().selectBrush();
	float threshold = brush.fuzzyColor().colorThreshold();
	if (ImGui::SliderFloat(_("Threshold"), &threshold, color::ApproximationDistanceMin,
						   color::ApproximationDistanceLoose, "%.0f")) {
		brush.fuzzyColor().setColorThreshold(threshold);
	}
	ImGui::TooltipTextUnformatted(
		_("Color distance threshold for fuzzy matching (0 = exact, higher = more similar colors)"));
}

void BrushPanelSelect::handleSelectFlatSurface(BrushPanelContext &ctx) {
	select::FlatSurface &flatSurface = ctx.sceneMgr->modifier().selectBrush().flatSurface();
	int deviation = flatSurface.deviation();
	const float btnW = ImGui::GetFrameHeight();
	const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	ImGui::TextUnformatted(_("Accepted deviation"));
	ImGui::TooltipTextUnformatted(
		_("How many voxels above or below the clicked face the fill may deviate from the start position"));
	ImGui::PushID("flatdeviation");
	if (ImGui::Button("-", ImVec2(btnW, 0))) {
		deviation = glm::max(deviation - 1, 0);
		flatSurface.setDeviation(deviation);
	}
	ImGui::SameLine(0, spacing);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
	if (ImGui::SliderInt("##flatdeviation", &deviation, 0, select::FlatSurface::MaxDeviation)) {
		flatSurface.setDeviation(deviation);
	}
	ImGui::SameLine(0, spacing);
	if (ImGui::Button("+", ImVec2(btnW, 0))) {
		deviation = glm::min(deviation + 1, select::FlatSurface::MaxDeviation);
		flatSurface.setDeviation(deviation);
	}
	ImGui::PopID();
}

void BrushPanelSelect::handleSelectLasso(command::CommandExecutionListener &listener) {
	ImGui::SeparatorText(_("Lasso selection"));
	ImGui::TextUnformatted(_("Click and drag to draw a selection polygon"));
}

void BrushPanelSelect::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	SelectBrush &brush = modifier.selectBrush();

	const SelectMode currentSelectMode = brush.selectMode();
	const bool luaModeActive = currentSelectMode == SelectMode::Script;
	const core::DynamicArray<LUASelectionMode *> &luaModes = modifier.scriptManager().luaSelectionModes();

	{
		ui::Toolbar toolbar("selectmode");
		// Native modes
		for (int i = 0; i < (int)SelectMode::Max; ++i) {
			if ((SelectMode)i == SelectMode::Script) {
				continue;
			}
			const bool active = !luaModeActive && (int)currentSelectMode == i;
			toolbar.button(
				SelectModeIcons[i], _(SelectModeStr[i]),
				[&brush, i]() {
					brush.setSelectMode((SelectMode)i);
					brush.setLuaSelectionMode(-1, nullptr);
				},
				!active);
		}
		// Lua selection modes
		for (int i = 0; i < (int)luaModes.size(); ++i) {
			const LUASelectionMode *mode = luaModes[i];
			const bool active = luaModeActive && brush.luaSelectionModeIndex() == i;
			toolbar.button(
				mode->iconString(), mode->scriptName().c_str(),
				[&brush, &luaModes, i]() { brush.setLuaSelectionMode(i, luaModes[i]); }, !active);
		}
	}

	const int nodeId = ctx.sceneMgr->sceneGraph().activeNode();
	ImGui::SeparatorText(_("Selection actions"));
	ImGui::CommandIconButton(ICON_LC_SCAN, _("Select Only Color"), "selectonlycolor", listener);
	ImGui::CommandIconButton(ICON_LC_PAINTBRUSH, _("Color Selected"), "colorselected", listener);
	ImGui::BeginDisabled(!ctx.sceneMgr->hasSelection(nodeId));
	ImGui::CommandIconButton(ICON_LC_SCAN, _("Deselect Color"), "deselectcolor", listener);
	ImGui::EndDisabled();

	if (brush.selectMode() == SelectMode::FuzzyColor) {
		handleSelectFuzzyColor(ctx);
	}

	if (brush.selectMode() == SelectMode::FlatSurface) {
		handleSelectFlatSurface(ctx);
	}

	if (brush.selectMode() == SelectMode::Lasso) {
		handleSelectLasso(listener);
	}

	if (brush.selectMode() == SelectMode::Paint) {
		handleSelectPaint(ctx, nodeId);
	}

	if (brush.selectMode() == SelectMode::Circle && brush.circle().valid() && ctx.sceneMgr->hasSelection(nodeId)) {
		handleSelectCircle(ctx, nodeId);
	}

	if (brush.selectMode() == SelectMode::Box3D) {
		handleSelectBox3D(ctx, nodeId);
	}

	if (brush.selectMode() == SelectMode::Script) {
		LUASelectionMode *luaMode = brush.activeLuaSelectionMode();
		if (!luaMode->scriptDescription().empty()) {
			ImGui::TextWrappedUnformatted(luaMode->scriptDescription().c_str());
		}
		voxelui::renderScriptParameters(luaMode->parameterDescriptions(), luaMode->parameters());
	}
}

} // namespace voxedit
