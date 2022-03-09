/**
 * @file
 */

#include "FormatSettingsPanel.h"
#include "imgui.h"
#include "ui/imgui/IMGUIEx.h"

namespace voxedit {

void FormatSettingsPanel::update(const char *title) {
	if (ImGui::Begin(title)) {
		if (ImGui::CollapsingHeader("Mesh import/export settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputVarFloat("Uniform scale", cfg::VoxformatScale);
			ImGui::InputVarFloat("X axis scale", cfg::VoxformatScaleX);
			ImGui::InputVarFloat("Y axis scale", cfg::VoxformatScaleY);
			ImGui::InputVarFloat("Z axis scale", cfg::VoxformatScaleZ);
			ImGui::CheckboxVar("Ambient occlusion", cfg::VoxformatAmbientocclusion);
			ImGui::CheckboxVar("Merge quads", cfg::VoxformatMergequads);
			ImGui::CheckboxVar("Reuse vertices", cfg::VoxformatReusevertices);
			ImGui::CheckboxVar("Apply transformations", cfg::VoxformatTransform);
			ImGui::CheckboxVar("Exports quads", cfg::VoxformatQuads);
			ImGui::CheckboxVar("Vertex colors", cfg::VoxformatWithcolor);
			ImGui::CheckboxVar("Texture coordinates", cfg::VoxformatWithtexcoords);
		}
		if (ImGui::CollapsingHeader("Voxel load/save settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputVarInt("Frame", cfg::VoxformatFrame);
		}
	}
	ImGui::End();
}

}
