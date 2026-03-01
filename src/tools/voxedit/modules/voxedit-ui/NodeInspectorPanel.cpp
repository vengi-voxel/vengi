/**
 * @file
 */

#include "NodeInspectorPanel.h"
#include "ViewMode.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphUtil.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "ui/dearimgui/implot.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/SceneManager.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/scalar_constants.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

bool NodeInspectorPanel::init() {
	_regionSizes = core::getVar(cfg::VoxEditRegionSizes);
	_localSpace = core::getVar(cfg::VoxEditLocalSpace);
	_gridSize = core::getVar(cfg::VoxEditGridsize);
	_viewMode = core::getVar(cfg::VoxEditViewMode);
	return true;
}

void NodeInspectorPanel::shutdown() {
}

void NodeInspectorPanel::modelRegions(command::CommandExecutionListener &listener, scenegraph::SceneGraphNode &node) {
	if (ImGui::IconCollapsingHeader(ICON_LC_RULER, _("Region"), ImGuiTreeNodeFlags_DefaultOpen)) {
		static const char *max = "888x888x888";
		const ImVec2 buttonSize(ImGui::CalcTextSize(max).x, ImGui::GetFrameHeight());
		ui::Toolbar toolbar("toolbar", buttonSize, &listener);

		for (glm::ivec3 maxs : _validRegionSizes) {
			const core::String &title = core::String::format("%ix%ix%i##regionsize", maxs.x, maxs.y, maxs.z);
			toolbar.button([&](const ImVec2 &) {
				if (ImGui::Button(title.c_str())) {
					voxel::Region newRegion(glm::ivec3(0), maxs - 1);
					_sceneMgr->nodeResize(node.id(), newRegion);
				}
			}, false);
		}
	}
}

void NodeInspectorPanel::modelProperties(scenegraph::SceneGraphNode &node) {
	const voxel::Region &region = node.region();
	if (!region.isValid()) {
		return;
	}
	static const uint32_t tableFlags =
		ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings;
	ui::ScopedStyle style;
	style.setIndentSpacing(0.0f);
	if (ImGui::BeginTable("##volume_props", 2, tableFlags)) {
		const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
								  ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(_("Name"), colFlags);
		ImGui::TableHeadersRow();

		glm::ivec3 position = region.getLowerCorner();
		const int minStep = _gridSize->intVal();
		const int maxStep = 10;
		const bool posChange = ImGui::InputXYZ(_("Position"), position, nullptr, ImGuiInputTextFlags_None, minStep, maxStep);
		if (posChange || ImGui::IsItemDeactivatedAfterEdit()) {
			const glm::ivec3 &f = position - region.getLowerCorner();
			_sceneMgr->nodeShift(node.id(), f);
		}

		glm::ivec3 dimensions = region.getDimensionsInVoxels();
		const bool sizeChange = ImGui::InputXYZ(_("Size"), dimensions, nullptr, ImGuiInputTextFlags_None, minStep, maxStep);
		if (sizeChange || ImGui::IsItemDeactivatedAfterEdit()) {
			voxel::Region newRegion(region.getLowerCorner(), region.getLowerCorner() + dimensions - 1);
			_sceneMgr->nodeResize(node.id(), newRegion);
		}
		ImGui::EndTable();
	}
}

void NodeInspectorPanel::saveRegionSizes(const core::Buffer<glm::ivec3> &sizes) {
	core::Set<glm::ivec3, 11, glm::hash<glm::ivec3>> uniqueSizes;
	for (const glm::ivec3 &maxs : sizes) {
		uniqueSizes.insert(maxs);
	}

	core::String valStr;
	for (const auto &e : uniqueSizes) {
		const glm::ivec3 &maxs = e->key;
		if (!valStr.empty()) {
			valStr += ",";
		}
		if (maxs.x <= 0 || maxs.x > MaxVolumeSize || maxs.y <= 0 || maxs.y > MaxVolumeSize || maxs.z <= 0 ||
			maxs.z > MaxVolumeSize) {
			Log::warn("Invalid region size %ix%ix%i", maxs.x, maxs.y, maxs.z);
			continue;
		}
		valStr += core::String::format("%i %i %i", maxs.x, maxs.y, maxs.z);
	}
	_regionSizes->setVal(valStr);
	_validRegionSizes.clear();
}

void NodeInspectorPanel::modelViewMenuBar(scenegraph::SceneGraphNode &node) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Tools"))) {
			const voxel::Region &region = node.region();
			const glm::ivec3 &mins = region.getLowerCorner();
			ImGui::BeginDisabled(!region.isValid() || (mins.x == 0 && mins.y == 0 && mins.z == 0));
			if (ImGui::IconButton(ICON_LC_MOVE_3D, _("To transform"))) {
				_sceneMgr->nodeShiftAllKeyframes(node.id(), mins);
				_sceneMgr->nodeShift(node.id(), -mins);
			}
			ImGui::TooltipTextUnformatted(_("Convert the region offset into the keyframe transforms"));
			ImGui::EndDisabled();
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_CAMERA, _("Options"))) {
			if (ImGui::BeginIconMenu(ICON_LC_SIGMA, _("Region sizes"))) {
				int removeIdx = -1;
				for (size_t i = 0; i < _validRegionSizes.size(); ++i) {
					const glm::ivec3 &maxs = _validRegionSizes[i];
					const core::String &title = core::String::format("%ix%ix%i##regionsize", maxs.x, maxs.y, maxs.z);
					if (ImGui::Selectable(title.c_str())) {
						removeIdx = (int)i;
					}
				}
				if (removeIdx != -1) {
					_validRegionSizes.erase(removeIdx);
					saveRegionSizes(_validRegionSizes);
				}
				ImGui::InputXYZ("##newregion", _newRegionSize);
				if (ImGui::MenuItem(_("Add"))) {
					_validRegionSizes.push_back(_newRegionSize);
					saveRegionSizes(_validRegionSizes);
				}
				ImGui::EndMenu();
			}
			if (!_validRegionSizes.empty()) {
				char cmdBuffer[64];
				core::String::formatBuf(cmdBuffer, sizeof(cmdBuffer), "clear %s", cfg::VoxEditRegionSizes);
				ImGui::CommandIconMenuItem(ICON_LC_X, _("Reset region sizes"), cmdBuffer);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void NodeInspectorPanel::modelView(command::CommandExecutionListener &listener) {
	core_trace_scoped(ModelView);
	updateModelRegionSizes();
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
		modelViewMenuBar(*node);
		if (!_validRegionSizes.empty()) {
			modelRegions(listener, *node);
		} else {
			modelProperties(*node);
		}
	}
}

void NodeInspectorPanel::keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node,
													   scenegraph::KeyFrameIndex keyFrameIdx) {
	ImGui::BeginDisabled(node.type() == scenegraph::SceneGraphNodeType::Camera);
	const scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	const int currentInterpolation = (int)keyFrame.interpolation;
	if (ImGui::BeginCombo(_("Interpolation"), scenegraph::InterpolationTypeStr[currentInterpolation])) {
		for (int n = 0; n < lengthof(scenegraph::InterpolationTypeStr); n++) {
			const bool isSelected = (currentInterpolation == n);
			if (ImGui::Selectable(scenegraph::InterpolationTypeStr[n], isSelected)) {
				_sceneMgr->nodeUpdateKeyFrameInterpolation(node.id(), keyFrameIdx, (scenegraph::InterpolationType)n);
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IconCollapsingHeader(ICON_LC_CHART_LINE, _("Interpolation details"))) {
		core::Array<glm::dvec2, 20> data;
		for (size_t i = 0; i < data.size(); ++i) {
			const double t = (double)i / (double)data.size();
			const double v = scenegraph::interpolate(keyFrame.interpolation, t, 0.0, 1.0);
			data[i] = glm::dvec2(t, v);
		}
		ImPlotFlags flags = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs;
		if (ImPlot::BeginPlot("##plotintertype", ImVec2(-1, 0), flags)) {
			ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
			ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
			ImPlot::SetupAxisLimits(ImAxis_X1, 0.0f, 1.0f, ImGuiCond_Once);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 1.0f, ImGuiCond_Once);
			const char *lineTitle = scenegraph::InterpolationTypeStr[currentInterpolation];
			ImPlotSpec spec;
			spec.Flags = ImPlotLineFlags_None;
			spec.Offset = 0;
			spec.Stride = sizeof(glm::dvec2);
			ImPlot::PlotLine(lineTitle, &data[0].x, &data[0].y, (int)data.size(), spec);
			ImPlot::EndPlot();
		}
	}
	ImGui::EndDisabled();
}

void NodeInspectorPanel::sceneViewMenuBar(scenegraph::SceneGraphNode &node) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_CAMERA, _("Tools"))) {
			ImGui::CommandIconMenuItem(ICON_LC_X, _("Reset transforms"), "transformreset");
			char cmdBuffer[64];
			core::String::formatBuf(cmdBuffer, sizeof(cmdBuffer), "transformmirror x %i", node.id());
			ImGui::CommandIconMenuItem(ICON_LC_FLIP_HORIZONTAL_2, _("Mirror X"), cmdBuffer);

			core::String::formatBuf(cmdBuffer, sizeof(cmdBuffer), "transformmirror y %i", node.id());
			ImGui::CommandIconMenuItem(ICON_LC_FLIP_VERTICAL_2, _("Mirror Y"), cmdBuffer);

			core::String::formatBuf(cmdBuffer, sizeof(cmdBuffer), "transformmirror xz %i", node.id());
			ImGui::CommandIconMenuItem(_("XZ"), _("Mirror XZ"), cmdBuffer);

			core::String::formatBuf(cmdBuffer, sizeof(cmdBuffer), "transformmirror xyz %i", node.id());
			ImGui::CommandIconMenuItem(_("XYZ"), _("Mirror XYZ"), cmdBuffer);
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
			ImGui::CheckboxVar(_localSpace);
			ImGui::CheckboxVar(cfg::VoxEditTransformUpdateChildren);
			ImGui::CheckboxVar(cfg::VoxEditAutoKeyFrame);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void NodeInspectorPanel::sceneView(command::CommandExecutionListener &listener, scenegraph::SceneGraphNode &node) {
	core_trace_scoped(SceneView);

	sceneViewMenuBar(node);

	bool change = false;
	bool changeMultiple = false;
	bool pivotChanged = false;

	const scenegraph::FrameIndex frameIdx = _sceneMgr->currentFrame();
	scenegraph::KeyFrameIndex keyFrameIdx = node.keyFrameForFrame(frameIdx);
	scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
	glm::vec3 matrixTranslation = _localSpace->boolVal() ? transform.localTranslation() : transform.worldTranslation();
	glm::vec3 matrixScale = _localSpace->boolVal() ? transform.localScale() : transform.worldScale();
	glm::quat matrixOrientation = _localSpace->boolVal() ? transform.localOrientation() : transform.worldOrientation();
	glm::vec3 matrixRotation = glm::degrees(glm::eulerAngles(matrixOrientation));
	glm::vec3 pivot = node.pivot();
	static const uint32_t tableFlags =
		ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings;
	ui::ScopedStyle style;
	style.setIndentSpacing(0.0f);
	if (ImGui::BeginTable("##node_props", 4, tableFlags)) {
		const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
								  ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn(ICON_LC_X "##reset", colFlags);
		ImGui::TableSetupColumn(ICON_LC_LOCK "##lock", colFlags);
		ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(_("Name"), colFlags);
		ImGui::TableHeadersRow();
		const char *reset = _("Reset");
		const char *updateTooltip = _("Update all locked nodes with this value");

		if (_localSpace->boolVal()) {
			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_X "##resettr")) {
				matrixTranslation[0] = matrixTranslation[1] = matrixTranslation[2] = 0.0f;
				change = true;
			}
			ImGui::TooltipTextUnformatted(reset);

			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_LOCK "##multipletr")) {
				changeMultiple = true;
			}
			ImGui::TooltipTextUnformatted(updateTooltip);
		} else {
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
		}
		const float minStep = _gridSize->floatVal();
		const float maxStep = 10.0f;
		change |= ImGui::InputXYZ(_("Translation"), matrixTranslation, nullptr, ImGuiInputTextFlags_None, minStep, maxStep);
		change |= ImGui::IsItemDeactivatedAfterEdit();

		// ------------------

		if (_localSpace->boolVal()) {
			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_X "##resetrt")) {
				matrixRotation[0] = matrixRotation[1] = matrixRotation[2] = 0.0f;
				change = true;
			}
			ImGui::TooltipTextUnformatted(reset);

			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_LOCK "##multiplert")) {
				changeMultiple = true;
			}
			ImGui::TooltipTextUnformatted(updateTooltip);
		} else {
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
		}
		change |= ImGui::InputXYZ(_("Rotation"), matrixRotation, nullptr, ImGuiInputTextFlags_None, 0.1f, 1.0f);
		change |= ImGui::IsItemDeactivatedAfterEdit();

		// ------------------

		if (_localSpace->boolVal()) {
			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_X "##resetsc")) {
				matrixScale[0] = matrixScale[1] = matrixScale[2] = 1.0f;
				change = true;
			}
			ImGui::TooltipTextUnformatted(reset);

			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_LOCK "##multiplesc")) {
				changeMultiple = true;
			}
			ImGui::TooltipTextUnformatted(updateTooltip);
		} else {
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
		}
		change |= ImGui::InputXYZ(_("Scale"), matrixScale, nullptr, ImGuiInputTextFlags_None, 0.1f, 1.0f);
		change |= ImGui::IsItemDeactivatedAfterEdit();

		// ------------------

		ImGui::TableNextColumn();
		if (ImGui::Button(ICON_LC_X "##resetpv")) {
			pivot[0] = pivot[1] = pivot[2] = 0.0f;
			pivotChanged = change = true;
		}
		ImGui::TooltipTextUnformatted(reset);
		ImGui::TableNextColumn();
		if (ImGui::Button(ICON_LC_LOCK "##multiplepv")) {
			_sceneMgr->nodeGroupUpdatePivot(pivot);
		}
		ImGui::TooltipTextUnformatted(updateTooltip);
		pivotChanged |= ImGui::InputXYZ(_("Pivot"), pivot, nullptr, ImGuiInputTextFlags_None, 0.1f, 0.1f);
		pivotChanged |= ImGui::IsItemDeactivatedAfterEdit();
		change |= pivotChanged;

		ImGui::EndTable();
	}

	keyFrameInterpolationSettings(node, keyFrameIdx);

	if (viewModeAnimations(_viewMode->intVal())) {
		ikConstraintSettings(node);
	}

	if (change) {
		const bool autoKeyFrame = core::getVar(cfg::VoxEditAutoKeyFrame)->boolVal();
		// check if a new keyframe should get generated automatically
		if (autoKeyFrame && node.keyFrame(keyFrameIdx).frameIdx != frameIdx) {
			if (_sceneMgr->nodeAddKeyFrame(node.id(), frameIdx)) {
				const scenegraph::KeyFrameIndex newKeyFrameIdx = node.keyFrameForFrame(frameIdx);
				core_assert(newKeyFrameIdx != keyFrameIdx);
				core_assert(newKeyFrameIdx != InvalidKeyFrame);
				keyFrameIdx = newKeyFrameIdx;
			}
		}
		if (pivotChanged) {
			_sceneMgr->nodeUpdatePivot(node.id(), pivot);
		} else {
			_sceneMgr->nodeUpdateTransform(node.id(), matrixRotation, matrixScale, matrixTranslation, keyFrameIdx,
										   _localSpace->boolVal());
		}
	} else if (changeMultiple) {
		_sceneMgr->nodeGroupUpdateTransform(matrixRotation, matrixScale, matrixTranslation, frameIdx,
											_localSpace->boolVal());
	}
}

void NodeInspectorPanel::ikConstraintSettings(scenegraph::SceneGraphNode &node) {
	if (!ImGui::IconCollapsingHeader(ICON_LC_BONE, _("IK Constraints"))) {
		return;
	}

	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const bool hasConstraint = node.hasIKConstraint();
	scenegraph::IKConstraint constraint;
	if (hasConstraint) {
		constraint = *node.ikConstraint();
	}

	bool changed = false;

	// Enable/disable IK constraint
	bool enabled = hasConstraint;
	if (ImGui::Checkbox(_("Enable IK"), &enabled)) {
		if (enabled && !hasConstraint) {
			_sceneMgr->nodeSetIKConstraint(node.id(), constraint);
		} else if (!enabled && hasConstraint) {
			_sceneMgr->nodeRemoveIKConstraint(node.id());
		}
		return;
	}

	if (!enabled) {
		return;
	}

	// Anchor toggle
	if (ImGui::Checkbox(_("Anchor"), &constraint.anchor)) {
		changed = true;
	}
	ImGui::TooltipTextUnformatted(_("Mark this node as a fixed point in the IK chain"));

	// Visibility toggle
	if (ImGui::Checkbox(_("Visible"), &constraint.visible)) {
		changed = true;
	}

	// Effector node selection
	const char *effectorName = _("None");
	if (constraint.effectorNodeId != InvalidNodeId && sceneGraph.hasNode(constraint.effectorNodeId)) {
		effectorName = sceneGraph.node(constraint.effectorNodeId).name().c_str();
	}
	if (ImGui::BeginCombo(_("Effector"), effectorName)) {
		// Option to clear the effector
		if (ImGui::Selectable(_("None"), constraint.effectorNodeId == InvalidNodeId)) {
			constraint.effectorNodeId = InvalidNodeId;
			changed = true;
		}
		// List all nodes that could be effectors
		for (auto iter = sceneGraph.beginAll(); iter != sceneGraph.end(); ++iter) {
			const scenegraph::SceneGraphNode &candidate = *iter;
			if (candidate.id() == node.id()) {
				continue;
			}
			const bool isSelected = (constraint.effectorNodeId == candidate.id());
			if (ImGui::Selectable(candidate.name().c_str(), isSelected)) {
				constraint.effectorNodeId = candidate.id();
				changed = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::TooltipTextUnformatted(_("The target node the IK chain tries to reach"));

	// Roll limits
	float rollMinDeg = glm::degrees(constraint.rollMin);
	float rollMaxDeg = glm::degrees(constraint.rollMax);
	if (ImGui::DragFloat(_("Roll min"), &rollMinDeg, 1.0f, -180.0f, 180.0f, "%.1f deg")) {
		constraint.rollMin = glm::radians(rollMinDeg);
		changed = true;
	}
	if (ImGui::DragFloat(_("Roll max"), &rollMaxDeg, 1.0f, -180.0f, 180.0f, "%.1f deg")) {
		constraint.rollMax = glm::radians(rollMaxDeg);
		changed = true;
	}

	// Swing limits
	if (ImGui::IconCollapsingHeader(ICON_LC_CIRCLE, _("Swing Limits"))) {
		int removeIdx = -1;
		static const uint32_t swingTableFlags =
			ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings;
		if (constraint.swingLimits.size() > 0 && ImGui::BeginTable("##swinglimits", 4, swingTableFlags)) {
			const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
									  ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;
			ImGui::TableSetupColumn(_("Yaw"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn(_("Pitch"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn(_("Radius"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##delete", colFlags);
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < constraint.swingLimits.size(); ++i) {
				ImGui::PushID((int)i);
				auto &limit = constraint.swingLimits[i];
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				float yaw = glm::degrees(limit.center.x);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat("##yaw", &yaw, 1.0f, -180.0f, 180.0f, "%.1f deg")) {
					limit.center.x = glm::radians(yaw);
					changed = true;
				}

				ImGui::TableNextColumn();
				float pitch = glm::degrees(limit.center.y);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat("##pitch", &pitch, 1.0f, -180.0f, 180.0f, "%.1f deg")) {
					limit.center.y = glm::radians(pitch);
					changed = true;
				}

				ImGui::TableNextColumn();
				float radiusDeg = glm::degrees(limit.radius);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat("##radius", &radiusDeg, 1.0f, 0.0f, 180.0f, "%.1f deg")) {
					limit.radius = glm::radians(radiusDeg);
					changed = true;
				}

				ImGui::TableNextColumn();
				if (ImGui::Button(ICON_LC_TRASH)) {
					removeIdx = (int)i;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (removeIdx >= 0) {
			constraint.swingLimits.erase(removeIdx);
			changed = true;
		}
		if (ImGui::IconButton(ICON_LC_PLUS, _("Add swing limit"))) {
			scenegraph::IKConstraint::RadiusConstraint rc;
			rc.center = glm::vec2(0.0f);
			rc.radius = glm::half_pi<float>();
			constraint.swingLimits.push_back(rc);
			changed = true;
		}
	}

	if (changed) {
		_sceneMgr->nodeSetIKConstraint(node.id(), constraint);
	}
}

void NodeInspectorPanel::detailView(scenegraph::SceneGraphNode &node) {
	const core::String &uuidStr = node.uuid().str();
	ImGui::Text(_("UUID: %s"), uuidStr.c_str());

	core::String deleteKey;
	static const uint32_t tableFlags =
		ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings;
	ui::ScopedStyle style;
	style.setIndentSpacing(0.0f);
	if (ImGui::BeginTable("##nodelist", 3, tableFlags)) {
		const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
								  ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

		ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##nodepropertydelete", colFlags);
		ImGui::TableHeadersRow();

		ImGuiListClipper clipper;
		clipper.Begin(node.properties().size());
		while (clipper.Step()) {
			auto entry = core::next(node.properties().begin(), clipper.DisplayStart);
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row, ++entry) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(entry->key.c_str());
				ImGui::TableNextColumn();
				bool propertyAlreadyHandled = false;

				if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
					propertyAlreadyHandled =
						handleCameraProperty(scenegraph::toCameraNode(node), entry->key, entry->value);
				}

				if (!propertyAlreadyHandled) {
					const core::String &id = core::String::format("##%i-%s", node.id(), entry->key.c_str());
					if (entry->value == "true" || entry->value == "false") {
						bool value = core::string::toBool(entry->value);
						if (ImGui::Checkbox(id.c_str(), &value)) {
							_sceneMgr->nodeSetProperty(node.id(), entry->key, value ? "true" : "false");
						}
					} else {
						core::String value = entry->value;
						if (ImGui::InputText(id.c_str(), &value,
											 ImGuiInputTextFlags_EnterReturnsTrue |
												 ImGuiInputTextFlags_AutoSelectAll)) {
							_sceneMgr->nodeSetProperty(node.id(), entry->key, value);
						}
					}
				}
				ImGui::TableNextColumn();
				const core::String &deleteId =
					core::String::format(ICON_LC_TRASH "##%i-%s-delete", node.id(), entry->key.c_str());
				if (ImGui::Button(deleteId.c_str())) {
					deleteKey = entry->key;
				}
				ImGui::TooltipTextUnformatted(_("Delete this node property"));
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::InputText("##newpropertykey", &_propertyKey);
		ImGui::TableNextColumn();
		ImGui::InputText("##newpropertyvalue", &_propertyValue);
		ImGui::TableNextColumn();
		if (ImGui::Button(ICON_LC_PLUS "###nodepropertyadd")) {
			_sceneMgr->nodeSetProperty(node.id(), _propertyKey, _propertyValue);
			_propertyKey = _propertyValue = "";
		}
		ImGui::TooltipTextUnformatted(_("Add a new node property"));

		ImGui::EndTable();
	}

	if (!deleteKey.empty()) {
		_sceneMgr->nodeRemoveProperty(node.id(), deleteKey);
	}
}

bool NodeInspectorPanel::handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key,
											  const core::String &value) {
	const core::String &id = core::String::format("##%i-%s", node.id(), key.c_str());
	if (key == scenegraph::PropCamMode) {
		int currentMode = value == scenegraph::SceneGraphNodeCamera::Modes[0] ? 0 : 1;

		if (ImGui::BeginCombo(id.c_str(), scenegraph::SceneGraphNodeCamera::Modes[currentMode])) {
			for (int n = 0; n < lengthof(scenegraph::SceneGraphNodeCamera::Modes); n++) {
				const bool isSelected = (currentMode == n);
				if (ImGui::Selectable(scenegraph::SceneGraphNodeCamera::Modes[n], isSelected)) {
					_sceneMgr->nodeSetProperty(node.id(), key, scenegraph::SceneGraphNodeCamera::Modes[n]);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	} else if (scenegraph::SceneGraphNodeCamera::isFloatProperty(key)) {
		float fvalue = core::string::toFloat(value);
		if (ImGui::InputFloat(id.c_str(), &fvalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			_sceneMgr->nodeSetProperty(node.id(), key, core::string::toString(fvalue));
		}
	} else if (scenegraph::SceneGraphNodeCamera::isIntProperty(key)) {
		int ivalue = core::string::toInt(value);
		if (ImGui::InputInt(id.c_str(), &ivalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			_sceneMgr->nodeSetProperty(node.id(), key, core::string::toString(ivalue));
		}
	} else {
		return false;
	}
	return true;
}

void NodeInspectorPanel::updateModelRegionSizes() {
	if (_regionSizes->isDirty()) {
		_validRegionSizes.clear();
		core::DynamicArray<core::String> strs;
		core::string::splitString(_regionSizes->strVal(), strs, ",");
		_validRegionSizes.reserve(strs.size());
		for (const core::String &s : strs) {
			glm::ivec3 maxs(0);
			core::string::parseIVec3(s, &maxs[0]);
			if (maxs.x <= 0 || maxs.x > MaxVolumeSize || maxs.y <= 0 || maxs.y > MaxVolumeSize || maxs.z <= 0 ||
				maxs.z > MaxVolumeSize) {
				continue;
			}
			_validRegionSizes.push_back(maxs);
		}
		_regionSizes->markClean();
	}
}

void NodeInspectorPanel::update(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(NodeInspectorPanel);
	const core::String title = makeTitle(ICON_LC_LOCATE, sceneMode ? _("Node Inspector") : _("Volume Inspector"), id);

	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
		if (sceneMode) {
			const int activeNode = _sceneMgr->sceneGraph().activeNode();
			if (activeNode != InvalidNodeId) {
				scenegraph::SceneGraphNode &node = _sceneMgr->sceneGraph().node(activeNode);
				sceneView(listener, node);
			}
		} else {
			modelView(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
