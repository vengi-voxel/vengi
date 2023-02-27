/**
 * @file
 */

#include "PositionsPanel.h"
#include "Toolbar.h"
#include "Util.h"
#include "core/Color.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IMGUIEx.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
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
				const glm::ivec3 maxs = core::string::parseIVec3(s);
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
					voxel::Region newRegion(region.getLowerCorner(), maxs - 1);
					sceneMgr().resize(nodeId, newRegion);
				}

				if (ImGui::CollapsingHeader(ICON_FA_CUBE " Gizmo settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::CheckboxVar("Show gizmo", cfg::VoxEditModelGizmo);
					ImGui::CheckboxVar("Flip Axis", cfg::VoxEditGizmoAllowAxisFlip);
					ImGui::CheckboxVar("Snap", cfg::VoxEditGizmoSnap);
				}
			}
		}
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_ARROW_UP " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
		static glm::ivec3 translate{0};
		veui::InputAxisInt(math::Axis::X, "X##translate", &translate.x, 1);
		veui::InputAxisInt(math::Axis::X, "Y##translate", &translate.y, 1);
		veui::InputAxisInt(math::Axis::X, "Z##translate", &translate.z, 1);
		if (ImGui::Button(ICON_FA_BORDER_ALL " Volumes")) {
			sceneMgr().shift(translate.x, translate.y, translate.z);
		}
		ImGui::TooltipText("Translate models by the given coordinates");
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_CUBES " Voxels")) {
			sceneMgr().move(translate.x, translate.y, translate.z);
		}
		ImGui::TooltipText("Translate the voxels without changing the volume boundaries");
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
		const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		const int activeNode = sceneGraph.activeNode();
		if (activeNode != -1) {
			voxelformat::SceneGraphNode &node = sceneGraph.node(activeNode);
			const uint32_t frame = sceneMgr().currentFrame();
			const uint32_t keyFrame = node.keyFrameForFrame(frame);
			voxelformat::SceneGraphKeyFrame &sceneGraphKeyFrame = node.keyFrame(keyFrame);
			voxelformat::SceneGraphTransform &transform = sceneGraphKeyFrame.transform();
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform.worldMatrix()), matrixTranslation, matrixRotation,
												  matrixScale);
			bool change = false;
			glm::vec3 pivot = transform.pivot();
			change |= ImGui::InputFloat3("Tr", matrixTranslation);
			change |= ImGui::InputFloat3("Rt", matrixRotation);
			change |= ImGui::InputFloat3("Sc", matrixScale);
			change |= ImGui::InputFloat3("Pv", glm::value_ptr(pivot));

			const int currentInterpolation = (int)sceneGraphKeyFrame.interpolation;
			if (ImGui::BeginCombo("Interpolation##interpolationstrings", voxelformat::InterpolationTypeStr[currentInterpolation])) {
				for (int n = 0; n < lengthof(voxelformat::InterpolationTypeStr); n++) {
					const bool isSelected = (currentInterpolation == n);
					if (ImGui::Selectable(voxelformat::InterpolationTypeStr[n], isSelected)) {
						sceneGraphKeyFrame.interpolation = (voxelformat::InterpolationType)n;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			if (change) {
				glm::mat4 matrix;
				_lastChanged = true;
				ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale,
														glm::value_ptr(matrix));
				transform.setWorldMatrix(matrix);
				transform.setPivot(pivot);
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
