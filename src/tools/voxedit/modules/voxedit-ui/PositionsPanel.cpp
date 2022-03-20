/**
 * @file
 */

#include "PositionsPanel.h"
#include "Util.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/dearimgui/ImGuizmo.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

void PositionsPanel::modelView(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(ICON_FA_ARROWS_ALT " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
		static glm::ivec3 translate{0};
		veui::InputAxisInt(math::Axis::X, "X##translate", &translate.x, 1);
		veui::InputAxisInt(math::Axis::X, "Y##translate", &translate.y, 1);
		veui::InputAxisInt(math::Axis::X, "Z##translate", &translate.z, 1);
		if (ImGui::Button(ICON_FA_BORDER_STYLE " Volumes")) {
			sceneMgr().shift(translate.x, translate.y, translate.z);
		}
		ImGui::TooltipText("Translate layers by the given coordinates");
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

void PositionsPanel::sceneView(command::CommandExecutionListener &listener, int frame) {
	if (ImGui::CollapsingHeader(ICON_FA_ARROWS_ALT " Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		const int activeNode = sceneGraph.activeNode();
		if (activeNode != -1) {
			voxelformat::SceneGraphNode &node = sceneGraph.node(activeNode);
			voxelformat::SceneGraphTransform &transform = node.transform(frame);
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform.matrix()), matrixTranslation, matrixRotation, matrixScale);
			bool change = false;
			change |= ImGui::InputFloat3("Tr", matrixTranslation);
			change |= ImGui::InputFloat3("Rt", matrixRotation);
			change |= ImGui::InputFloat3("Sc", matrixScale);
			if (change) {
				glm::mat4 matrix;
				ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(matrix));
				transform.setMatrix(matrix);
				transform.update();
			}
		}
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(ICON_FA_CUBE " Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::CheckboxVar("Flip Axis", cfg::VoxEditGuizmoAllowAxisFlip);
		ImGui::CheckboxVar("Activate rotate", cfg::VoxEditGuizmoRotation);
		ImGui::CheckboxVar("Snap", cfg::VoxEditGuizmoSnap);
	}
}

void PositionsPanel::update(const char *title, command::CommandExecutionListener &listener, int frame) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		if (sceneMgr().editMode() == EditMode::Scene) {
			sceneView(listener, frame);
		} else {
			modelView(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
