/**
 * @file
 */

#include "PositionsPanel.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "Util.h"
#include "core/Color.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IMGUIEx.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "core/ArrayLength.h"

#include <glm/gtc/type_ptr.hpp>

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
		style.setColor(ImGuiCol_Text, core::Color::Red);
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.x = core::string::toInt(buf);
		}
		ImGui::SameLine(0.0f, 2.0f);

		id.c_str()[id.size() - 1] = '1';
		core::string::formatBuf(buf, sizeof(buf), "%i", v.y);
		style.setColor(ImGuiCol_Text, core::Color::Green);
		ImGui::PushItemWidth(width);
		if (ImGui::InputText(id.c_str(), buf, sizeof(buf),
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			retVal = true;
			v.y = core::string::toInt(buf);
		}
		ImGui::SameLine(0.0f, 2.0f);

		id.c_str()[id.size() - 1] = '2';
		core::string::formatBuf(buf, sizeof(buf), "%i", v.z);
		style.setColor(ImGuiCol_Text, core::Color::Blue);
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

void PositionsPanel::modelView(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Region", ImGuiTreeNodeFlags_DefaultOpen)) {
		const int nodeId = sceneMgr().sceneGraph().activeNode();
		const core::String &sizes = core::Var::getSafe(cfg::VoxEditRegionSizes)->strVal();
		if (!sizes.empty()) {
			static const char *max = "888x888x888";
			const ImVec2 buttonSize(ImGui::CalcTextSize(max).x, ImGui::GetFrameHeight());
			ui::Toolbar toolbar(buttonSize, &listener);

			core::DynamicArray<core::String> regionSizes;
			core::string::splitString(sizes, regionSizes, ",");
			for (const core::String &s : regionSizes) {
				glm::ivec3 maxs;
				core::string::parseIVec3(s, &maxs[0]);
				for (int i = 0; i < 3; ++i) {
					if (maxs[i] <= 0 || maxs[i] > 256) {
						return;
					}
				}
				const core::String &title = core::string::format("%ix%ix%i##regionsize", maxs.x, maxs.y, maxs.z);
				toolbar.customNoStyle([&]() {
					if (ImGui::Button(title.c_str())) {
						voxel::Region newRegion(glm::ivec3(0), maxs);
						sceneMgr().resize(nodeId, newRegion);
					}
				});
			}
		} else {
			const voxel::RawVolume *v = sceneMgr().volume(nodeId);
			if (v != nullptr) {
				const voxel::Region &region = v->region();
				glm::ivec3 mins = region.getLowerCorner();
				glm::ivec3 maxs = region.getDimensionsInVoxels();
				if (xyzValues("pos", mins)) {
					const glm::ivec3 &f = mins - region.getLowerCorner();
					sceneMgr().shift(nodeId, f);
				}
				if (xyzValues("size", maxs)) {
					voxel::Region newRegion(region.getLowerCorner(), region.getLowerCorner() + maxs - 1);
					sceneMgr().resize(nodeId, newRegion);
				}

				if (ImGui::CollapsingHeader(ICON_FA_CUBE " Gizmo settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::CheckboxVar("Show gizmo", cfg::VoxEditModelGizmo);
					ImGui::CheckboxVar("Flip Axis", cfg::VoxEditGizmoAllowAxisFlip);
					ImGui::CheckboxVar("Snap", cfg::VoxEditGizmoSnap);
				}
			}
		}

		ImGui::SliderVarInt("Cursor details", cfg::VoxEditCursorDetails, 0, 2);
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_ARROW_UP " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
		static glm::ivec3 translate{0};
		veui::InputAxisInt(math::Axis::X, "X##translate", &translate.x, 1);
		veui::InputAxisInt(math::Axis::X, "Y##translate", &translate.y, 1);
		veui::InputAxisInt(math::Axis::X, "Z##translate", &translate.z, 1);
		const core::String &shiftCmd = core::string::format("shift %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandButton(ICON_FA_BORDER_ALL " Volumes", shiftCmd.c_str(), listener);
		ImGui::SameLine();
		const core::String &moveCmd = core::string::format("move %i %i %i", translate.x, translate.y, translate.z);
		ImGui::CommandButton(ICON_FA_CUBES " Voxels", moveCmd.c_str(), listener);
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_CUBE " Cursor", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::ivec3 cursorPosition = sceneMgr().modifier().cursorPosition();
		math::Axis lockedAxis = sceneMgr().lockedAxis();
		if (veui::CheckboxAxisFlags(math::Axis::X, "X##cursorlock", &lockedAxis)) {
			command::executeCommands("lockx", &listener);
		}
		ImGui::TooltipCommand("lockx");
		ImGui::SameLine();
		const int step = core::Var::getSafe(cfg::VoxEditGridsize)->intVal();
		if (veui::InputAxisInt(math::Axis::X, "##cursorx", &cursorPosition.x, step)) {
			sceneMgr().setCursorPosition(cursorPosition, true);
		}

		if (veui::CheckboxAxisFlags(math::Axis::Y, "Y##cursorlock", &lockedAxis)) {
			command::executeCommands("locky", &listener);
		}
		ImGui::TooltipCommand("locky");
		ImGui::SameLine();
		if (veui::InputAxisInt(math::Axis::Y, "##cursory", &cursorPosition.y, step)) {
			sceneMgr().setCursorPosition(cursorPosition, true);
		}

		if (veui::CheckboxAxisFlags(math::Axis::Z, "Z##cursorlock", &lockedAxis)) {
			command::executeCommands("lockz", &listener);
		}
		ImGui::TooltipCommand("lockz");
		ImGui::SameLine();
		if (veui::InputAxisInt(math::Axis::Z, "##cursorz", &cursorPosition.z, step)) {
			sceneMgr().setCursorPosition(cursorPosition, true);
		}
	}
}

void PositionsPanel::sceneView(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(ICON_FA_ARROW_UP " Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		const int activeNode = sceneGraph.activeNode();
		if (activeNode != InvalidNodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);

			const scenegraph::FrameIndex frame = sceneMgr().currentFrame();
			const scenegraph::KeyFrameIndex keyFrame = node.keyFrameForFrame(frame);
			scenegraph::SceneGraphKeyFrame &sceneGraphKeyFrame = node.keyFrame(keyFrame);
			scenegraph::SceneGraphTransform &transform = sceneGraphKeyFrame.transform();
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			const glm::mat4 &matrix = _localSpace ? transform.localMatrix() : transform.worldMatrix();
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), matrixTranslation, matrixRotation,
												  matrixScale);
			bool change = false;
			ImGui::Checkbox("Local transforms", &_localSpace);
			glm::vec3 pivot = node.pivot();
			change |= ImGui::InputFloat3("Tr", matrixTranslation, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resettr")) {
				matrixTranslation[0] = matrixTranslation[1] = matrixTranslation[2] = 0.0f;
				change = true;
			}
			ImGui::TooltipText("Reset");
			change |= ImGui::InputFloat3("Rt", matrixRotation, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resetrt")) {
				matrixRotation[0] = matrixRotation[1] = matrixRotation[2] = 0.0f;
				change = true;
			}
			ImGui::TooltipText("Reset");
			change |= ImGui::InputFloat3("Sc", matrixScale, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resetsc")) {
				matrixScale[0] = matrixScale[1] = matrixScale[2] = 1.0f;
				change = true;
			}
			ImGui::TooltipText("Reset");
			bool pivotChanged = ImGui::InputFloat3("Pv", glm::value_ptr(pivot), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
			change |= pivotChanged;
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_X "##resetpv")) {
				pivot[0] = pivot[1] = pivot[2] = 0.0f;
				pivotChanged = change = true;
			}
			ImGui::TooltipText("Reset");

			if (ImGui::Button("Reset all")) {
				if (_localSpace) {
					transform.setLocalMatrix(glm::mat4(1.0f));
				} else {
					transform.setWorldMatrix(glm::mat4(1.0f));
				}
				node.setPivot({0.0f, 0.0f, 0.0f});
				transform.update(sceneGraph, node, frame);
				sceneMgr().mementoHandler().markNodeTransform(node, keyFrame);
			}

			{
				ui::ScopedStyle style;
				if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
					style.disableItem();
				}
				const int currentInterpolation = (int)sceneGraphKeyFrame.interpolation;
				if (ImGui::BeginCombo("Interpolation##interpolationstrings", scenegraph::InterpolationTypeStr[currentInterpolation])) {
					for (int n = 0; n < lengthof(scenegraph::InterpolationTypeStr); n++) {
						const bool isSelected = (currentInterpolation == n);
						if (ImGui::Selectable(scenegraph::InterpolationTypeStr[n], isSelected)) {
							sceneGraphKeyFrame.interpolation = (scenegraph::InterpolationType)n;
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
			if (change) {
				glm::mat4 matrix;
				_lastChanged = true;

				if (pivotChanged) {
					const glm::vec3 oldPivot = node.pivot();
					const glm::vec3 deltaPivot = oldPivot - pivot;
					const glm::vec3 size = node.region().getDimensionsInVoxels();
					if (node.setPivot(pivot)) {
						for (int i = 0; i < 3; ++i) {
							matrixTranslation[i] -= deltaPivot[i] * size[i];
						}
					}

					for (int nodeId : node.children()) {
						sceneGraph.node(nodeId).translate(deltaPivot * size);
					}
				}

				ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale,
														glm::value_ptr(matrix));
				if (_localSpace) {
					transform.setLocalMatrix(matrix);
				} else {
					transform.setWorldMatrix(matrix);
				}

				transform.update(sceneGraph, node, frame);
			}
			if (!change && _lastChanged) {
				_lastChanged = false;
				sceneMgr().mementoHandler().markNodeTransform(node, keyFrame);
			}
		}
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_CUBE " Gizmo settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::CheckboxVar("Show gizmo", cfg::VoxEditShowaxis);
		ImGui::CheckboxVar("Flip Axis", cfg::VoxEditGizmoAllowAxisFlip);
		ImGui::CheckboxVar("Activate rotate", cfg::VoxEditGizmoRotation);
		ImGui::CheckboxVar("Size", cfg::VoxEditGizmoBounds);
		ImGui::CheckboxVar("Snap", cfg::VoxEditGizmoSnap);
	}
}

void PositionsPanel::update(const char *title, bool sceneMode, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			sceneView(listener);
		} else {
			modelView(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
