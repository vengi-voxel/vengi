/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

static void recursiveAddNodes(const voxel::SceneGraph &sceneGraph, const voxel::SceneGraphNode &node) {
	core::String name;
	switch (node.type()) {
	case voxel::SceneGraphNodeType::Model:
		name = ICON_FA_CUBES;
		break;
	case voxel::SceneGraphNodeType::ModelReference:
		name = ICON_FA_CUBE;
		break;
	case voxel::SceneGraphNodeType::Root:
	case voxel::SceneGraphNodeType::Group:
		name = ICON_FA_OBJECT_GROUP;
		break;
	case voxel::SceneGraphNodeType::Camera:
		name = ICON_FA_CAMERA;
		break;
	case voxel::SceneGraphNodeType::Transform:
		name = ICON_FA_SLIDERS_H;
		break;
	case voxel::SceneGraphNodeType::Unknown:
		name = ICON_FA_QUESTION_CIRCLE;
		break;
	case voxel::SceneGraphNodeType::Max:
		break;
	}
	name.append(core::string::format(" %s##%i", node.name().c_str(), node.id()));
	if (ImGui::TreeNode(name.c_str())) {
		ImGui::TooltipText("Node id: %i", node.id());
		if (node.type() == voxel::SceneGraphNodeType::Camera) {
			if (ImGui::IsItemClicked() /*&& !ImGui::IsItemToggledOpen()*/) {
				video::Camera camera;
				camera.setQuaternion(glm::quat_cast(node.matrix()));
				// TODO: switch to this camera when double clicking this...
			}
		}
		if (node.type() == voxel::SceneGraphNodeType::Model) {
			const voxel::Region &region = node.region();
			const glm::ivec3 &pos = region.getLowerCorner();
			const glm::ivec3 &size = region.getDimensionsInVoxels();
			ImGui::LabelText(core::string::format("%i:%i:%i", pos.x, pos.y, pos.z).c_str(), "position");
			ImGui::LabelText(core::string::format("%i:%i:%i", size.x, size.y, size.z).c_str(), "size");
			if (ImGui::IsItemClicked() /*&& !ImGui::IsItemToggledOpen()*/) {
				// TODO: switch to scene mode and select node
			}
		}
		for (const auto& entry : node.properties()) {
			// TODO: allow to edit them
			ImGui::LabelText(entry->value.c_str(), "%s", entry->key.c_str());
		}
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(sceneGraph, sceneGraph.node(nodeIdx));
		}
		ImGui::TreePop();
	}
}

void SceneGraphPanel::update(const char *title) {
	const voxel::SceneGraph &sceneGraph = voxedit::sceneMgr().sceneGraph();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(SceneGraphPanel);
		ImGui::SetNextItemOpen(true);
		recursiveAddNodes(sceneGraph, sceneGraph.node(0));
	}
	ImGui::End();
}

} // namespace voxedit
