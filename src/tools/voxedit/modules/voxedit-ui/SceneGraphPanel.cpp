/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

static void recursiveAddNodes(voxel::SceneGraph &sceneGraph, voxel::SceneGraphNode &node) {
	core::String name;
	switch (node.type()) {
	case voxel::SceneGraphNodeType::Model:
		name = ICON_FA_CUBES;
		break;
	case voxel::SceneGraphNodeType::Root:
		name = ICON_FA_OBJECT_GROUP;
		break;
	case voxel::SceneGraphNodeType::Max:
		break;
	}
	if (node.visible()) {
		name.append(ICON_FA_EYE);
	} else {
		name.append(ICON_FA_EYE_SLASH);
	}
	if (node.locked()) {
		name.append(ICON_FA_LOCK);
	} else {
		name.append(ICON_FA_LOCK_OPEN);
	}
	name.append(node.name());
	if (ImGui::TreeNode(name.c_str())) {
		for (const auto& entry : node.properties()) {
			ImGui::LabelText(entry->value.c_str(), "%s", entry->key.c_str());
		}
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(sceneGraph, sceneGraph.node(nodeIdx));
		}
		ImGui::TreePop();
	}
}

void SceneGraphPanel::update(const char *title) {
	voxel::SceneGraph &sceneGraph = voxedit::sceneMgr().sceneGraph();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(SceneGraphPanel);
		recursiveAddNodes(sceneGraph, sceneGraph.node(0));
	}
	ImGui::End();
}

} // namespace voxedit
