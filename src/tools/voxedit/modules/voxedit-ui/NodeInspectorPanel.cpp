/**
 * @file
 */

#include "NodeInspectorPanel.h"
#include "Util.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "Gizmo.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "ui/dearimgui/implot.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

static bool xyzValues(const char *title, glm::ivec3 &v) {
	bool retVal = false;
	const float width = ImGui::CalcTextSize("10000").x + ImGui::GetStyle().FramePadding.x * 2.0f;

	char buf[64];
	core::String id = "##";
	id.append(title);
	id.append("0");

	id.c_str()[id.size() - 1] = '0';
	core::string::formatBuf(buf, sizeof(buf), "%i", v.x);
	{
		ui::ScopedStyle style;
		style.setColor(ImGuiCol_Text, core::Color::Red());
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.x = core::string::toInt(buf);
		}
		ImGui::SameLine(0.0f, 2.0f);

		id.c_str()[id.size() - 1] = '1';
		core::string::formatBuf(buf, sizeof(buf), "%i", v.y);
		style.setColor(ImGuiCol_Text, core::Color::Green());
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.y = core::string::toInt(buf);
		}
		ImGui::SameLine(0.0f, 2.0f);

		id.c_str()[id.size() - 1] = '2';
		core::string::formatBuf(buf, sizeof(buf), "%i", v.z);
		style.setColor(ImGuiCol_Text, core::Color::Blue());
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.z = core::string::toInt(buf);
		}
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(title);

	return retVal;
}

bool NodeInspectorPanel::init() {
	_regionSizes = core::Var::getSafe(cfg::VoxEditRegionSizes);
	_localSpace = core::Var::getSafe(cfg::VoxEditLocalSpace);
	return true;
}

void NodeInspectorPanel::shutdown() {
}

void NodeInspectorPanel::modelView(command::CommandExecutionListener &listener) {
	core_trace_scoped(ModelView);
	if (ImGui::IconCollapsingHeader(ICON_LC_RULER, _("Region"), ImGuiTreeNodeFlags_DefaultOpen)) {
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		const core::String &sizes = _regionSizes->strVal();
		if (!sizes.empty()) {
			static const char *max = "888x888x888";
			const ImVec2 buttonSize(ImGui::CalcTextSize(max).x, ImGui::GetFrameHeight());
			ui::Toolbar toolbar("regions", buttonSize, &listener);

			core::DynamicArray<core::String> regionSizes;
			core::string::splitString(sizes, regionSizes, ",");
			for (const core::String &s : regionSizes) {
				glm::ivec3 maxs(0);
				core::string::parseIVec3(s, &maxs[0]);
				bool valid = true;
				for (int i = 0; i < 3; ++i) {
					if (maxs[i] <= 0 || maxs[i] > 256) {
						valid = false;
						break;
					}
				}
				if (!valid) {
					continue;
				}
				const core::String &title = core::string::format("%ix%ix%i##regionsize", maxs.x, maxs.y, maxs.z);
				toolbar.customNoStyle([&]() {
					if (ImGui::Button(title.c_str())) {
						voxel::Region newRegion(glm::ivec3(0), maxs - 1);
						_sceneMgr->nodeResize(nodeId, newRegion);
					}
				});
			}
		} else if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
			const voxel::RawVolume *v = node->volume();
			if (v != nullptr) {
				const voxel::Region &region = v->region();
				glm::ivec3 mins = region.getLowerCorner();
				glm::ivec3 maxs = region.getDimensionsInVoxels();
				if (xyzValues(_("pos"), mins)) {
					const glm::ivec3 &f = mins - region.getLowerCorner();
					_sceneMgr->nodeShift(nodeId, f);
				}
				if (mins.x != 0 || mins.y != 0 || mins.z != 0) {
					ImGui::SameLine();
					if (ImGui::Button(_("To transform"))) {
						const glm::ivec3 &f = region.getLowerCorner();
						_sceneMgr->nodeShiftAllKeyframes(nodeId, f);
						_sceneMgr->nodeShift(nodeId, -f);
					}
					ImGui::TooltipTextUnformatted(_("Convert the region offset into the keyframe transforms"));
				}
				if (xyzValues(_("Size"), maxs)) {
					voxel::Region newRegion(region.getLowerCorner(), region.getLowerCorner() + maxs - 1);
					_sceneMgr->nodeResize(nodeId, newRegion);
				}
			}
		}
	}
}

void NodeInspectorPanel::keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node,
												   scenegraph::KeyFrameIndex keyFrameIdx) {
	ui::ScopedStyle style;
	if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
		style.disableItem();
	}
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
			const ImPlotLineFlags lineFlag = ImPlotLineFlags_None;
			ImPlot::PlotLine(lineTitle, &data[0].x, &data[0].y, data.size(), lineFlag, 0, sizeof(glm::dvec2));
			ImPlot::EndPlot();
		}
	}
}

void NodeInspectorPanel::keyFrameActionsAndOptions(const scenegraph::SceneGraph &sceneGraph,
											   scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx,
											   scenegraph::KeyFrameIndex keyFrameIdx) {
	if (ImGui::Button(_("Reset all"))) {
		_sceneMgr->nodeResetTransform(node.id(), keyFrameIdx);
	}

	// TODO: icons and convert to commands
	if (ImGui::Button(_("Mirror X"))) {
		_sceneMgr->nodeTransformMirror(node.id(), keyFrameIdx, math::Axis::X);
	}

	if (ImGui::Button(_("Mirror XZ"))) {
		_sceneMgr->nodeTransformMirror(node.id(), keyFrameIdx, math::Axis::X | math::Axis::Z);
	}

	if (ImGui::Button(_("Mirror XYZ"))) {
		_sceneMgr->nodeTransformMirror(node.id(), keyFrameIdx, math::Axis::X | math::Axis::Y | math::Axis::Z);
	}
}

void NodeInspectorPanel::sceneView(command::CommandExecutionListener &listener) {
	core_trace_scoped(SceneView);
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	if (ImGui::IconCollapsingHeader(ICON_LC_ARROW_UP, _("Transform"), ImGuiTreeNodeFlags_DefaultOpen)) {
		const int activeNode = sceneGraph.activeNode();
		if (activeNode != InvalidNodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);

			const scenegraph::FrameIndex frameIdx = _sceneMgr->currentFrame();
			scenegraph::KeyFrameIndex keyFrameIdx = node.keyFrameForFrame(frameIdx);
			scenegraph::SceneGraphTransform &transform = node.keyFrame(keyFrameIdx).transform();
			glm::vec3 matrixTranslation = _localSpace->boolVal() ? transform.localTranslation() : transform.worldTranslation();
			glm::vec3 matrixScale = _localSpace->boolVal() ? transform.localScale() : transform.worldScale();
			glm::quat matrixOrientation = _localSpace->boolVal() ? transform.localOrientation() : transform.worldOrientation();
			glm::vec3 matrixRotation = glm::degrees(glm::eulerAngles(matrixOrientation));

			ImGui::CheckboxVar(_("Local transforms"), _localSpace);
			ImGui::CheckboxVar(_("Update children"), cfg::VoxEditTransformUpdateChildren);
			ImGui::CheckboxVar(_("Auto Keyframe"), cfg::VoxEditAutoKeyFrame);

			bool change = false;
			bool changeMultiple = false;
			ImGui::InputFloat3(_("Tr"), glm::value_ptr(matrixTranslation), "%.3f");
			change |= ImGui::IsItemDeactivatedAfterEdit();
			if (_localSpace->boolVal()) {
				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_X "##resettr")) {
					matrixTranslation[0] = matrixTranslation[1] = matrixTranslation[2] = 0.0f;
					change = true;
				}
				ImGui::TooltipTextUnformatted(_("Reset"));

				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_LOCK "##multipletr")) {
					changeMultiple = true;
				}
				ImGui::TooltipTextUnformatted(_("Update all locked nodes with this value"));
			}

			ImGui::InputFloat3(_("Rt"), glm::value_ptr(matrixRotation), "%.3f");
			change |= ImGui::IsItemDeactivatedAfterEdit();
			if (_localSpace->boolVal()) {
				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_X "##resetrt")) {
					matrixRotation[0] = matrixRotation[1] = matrixRotation[2] = 0.0f;
					change = true;
				}
				ImGui::TooltipTextUnformatted(_("Reset"));

				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_LOCK "##multiplert")) {
					changeMultiple = true;
				}
				ImGui::TooltipTextUnformatted(_("Update all locked nodes with this value"));
			}

			ImGui::InputFloat3(_("Sc"), glm::value_ptr(matrixScale), "%.3f");
			change |= ImGui::IsItemDeactivatedAfterEdit();
			if (_localSpace->boolVal()) {
				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_X "##resetsc")) {
					matrixScale[0] = matrixScale[1] = matrixScale[2] = 1.0f;
					change = true;
				}
				ImGui::TooltipTextUnformatted(_("Reset"));

				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_LOCK "##multiplesc")) {
					changeMultiple = true;
				}
				ImGui::TooltipTextUnformatted(_("Update all locked nodes with this value"));
			}

			glm::vec3 pivot = node.pivot();
			ImGui::InputFloat3(_("Pv"), glm::value_ptr(pivot), "%.3f");
			bool pivotChanged = ImGui::IsItemDeactivatedAfterEdit();
			change |= pivotChanged;
			ImGui::SameLine();
			if (ImGui::Button(ICON_LC_X "##resetpv")) {
				pivot[0] = pivot[1] = pivot[2] = 0.0f;
				pivotChanged = change = true;
			}
			ImGui::TooltipTextUnformatted(_("Reset"));
			ImGui::SameLine();
			if (ImGui::Button(ICON_LC_LOCK "##multiplepv")) {
				_sceneMgr->nodeUpdatePivotGroup(pivot);
			}
			ImGui::TooltipTextUnformatted(_("Update all locked nodes with this value"));
			keyFrameActionsAndOptions(sceneGraph, node, frameIdx, keyFrameIdx);
			keyFrameInterpolationSettings(node, keyFrameIdx);

			if (change) {
				const bool autoKeyFrame = core::Var::getSafe(cfg::VoxEditAutoKeyFrame)->boolVal();
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
					_sceneMgr->nodeUpdateTransform(node.id(), matrixRotation, matrixScale, matrixTranslation,
												   keyFrameIdx, _localSpace->boolVal());
				}
			} else if (changeMultiple) {
				_sceneMgr->nodeUpdateTransformGroup(matrixRotation, matrixScale, matrixTranslation, frameIdx, _localSpace->boolVal());
			}
		}
	}
	if (ImGui::IconCollapsingHeader(ICON_LC_ARROW_UP, _("Properties"), ImGuiTreeNodeFlags_DefaultOpen)) {
		detailView(sceneGraph.node(sceneGraph.activeNode()));
	}
}

void NodeInspectorPanel::detailView(scenegraph::SceneGraphNode &node) {
	ImGui::Text(_("UUID: %s"), node.uuid().c_str());

	core::String deleteKey;
	static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
										ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
										ImGuiTableFlags_NoSavedSettings;
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
					propertyAlreadyHandled = handleCameraProperty(scenegraph::toCameraNode(node), entry->key, entry->value);
				}

				if (!propertyAlreadyHandled) {
					const core::String &id = core::string::format("##%i-%s", node.id(), entry->key.c_str());
					if (entry->value == "true" || entry->value == "false") {
						bool value = core::string::toBool(entry->value);
						if (ImGui::Checkbox(id.c_str(), &value)) {
							_sceneMgr->nodeSetProperty(node.id(), entry->key, value ? "true" : "false");
						}
					} else {
						core::String value = entry->value;
						if (ImGui::InputText(id.c_str(), &value, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
							_sceneMgr->nodeSetProperty(node.id(), entry->key, value);
						}
					}
				}
				ImGui::TableNextColumn();
				const core::String &deleteId = core::string::format(ICON_LC_TRASH "##%i-%s-delete", node.id(), entry->key.c_str());
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

bool NodeInspectorPanel::handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key, const core::String &value) {
	const core::String &id = core::string::format("##%i-%s", node.id(), key.c_str());
	if (key == scenegraph::SceneGraphNodeCamera::PropMode) {
		int currentMode = value == scenegraph::SceneGraphNodeCamera::Modes[0] ? 0 : 1;

		if (ImGui::BeginCombo(id.c_str(), scenegraph::SceneGraphNodeCamera::Modes[currentMode])) {
			for (int n = 0; n < IM_ARRAYSIZE(scenegraph::SceneGraphNodeCamera::Modes); n++) {
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

void NodeInspectorPanel::update(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(NodeInspectorPanel);
	const core::String title = makeTitle(ICON_LC_LOCATE, sceneMode ? _("Node Inspector") : _("Volume Inspector"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			sceneView(listener);
		} else {
			modelView(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
