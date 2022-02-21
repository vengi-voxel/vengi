/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "ui/imgui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

#define SCENEGRAPHPOPUP "##scenegraphpopup"

namespace voxedit {

static void recursiveAddNodes(video::Camera& camera, const voxel::SceneGraph &sceneGraph, const voxel::SceneGraphNode &node, command::CommandExecutionListener &listener) {
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
	case voxel::SceneGraphNodeType::Unknown:
		name = ICON_FA_QUESTION_CIRCLE;
		break;
	case voxel::SceneGraphNodeType::Max:
		break;
	}
	name.append(core::string::format(" %s##%i", node.name().c_str(), node.id()));
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	const bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
	if (node.id() != sceneGraph.root().id()) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::Text("%s", name.c_str());
			const int sourceNodeId = node.id();
			ImGui::SetDragDropPayload("scenegraphnode", (const void*)&sourceNodeId, sizeof(int), ImGuiCond_Always);
			ImGui::EndDragDropSource();
		}
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("scenegraphnode")) {
			const int sourceNodeId = *(int*)payload->Data;
			const int targetNode = node.id();
			if (!sceneMgr().nodeMove(sourceNodeId, targetNode)) {
				Log::error("Failed to move node");
			}
		}
		ImGui::EndDragDropTarget();
	}

	const core::String &contextMenuId = core::string::format("Edit##context-node-%i", node.id());
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		const int validLayers = (int)sceneGraph.size();
		sceneMgr().nodeActivate(node.id());
		ImGui::CommandMenuItem(ICON_FA_TRASH_ALT " Delete" SCENEGRAPHPOPUP, "layerdelete", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_EYE_SLASH " Hide others" SCENEGRAPHPOPUP, "layerhideothers", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_COPY " Duplicate" SCENEGRAPHPOPUP, "layerduplicate", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_EYE " Show all" SCENEGRAPHPOPUP, "layershowall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_EYE_SLASH " Hide all" SCENEGRAPHPOPUP, "layerhideall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge" SCENEGRAPHPOPUP, "layermerge", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge all" SCENEGRAPHPOPUP, "layermergeall", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge visible" SCENEGRAPHPOPUP, "layermergevisible", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge locked" SCENEGRAPHPOPUP, "layermergelocked", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_LOCK " Lock all" SCENEGRAPHPOPUP, "layerlockall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_UNLOCK " Unlock all" SCENEGRAPHPOPUP, "layerunlockall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_COMPRESS_ARROWS_ALT " Center origin" SCENEGRAPHPOPUP, "center_origin", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_COMPRESS_ARROWS_ALT " Center reference" SCENEGRAPHPOPUP, "center_referenceposition", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_SAVE " Save" SCENEGRAPHPOPUP, "layerssave", true, &listener);
		core::String layerName = node.name();
		if (ImGui::InputText("Name" SCENEGRAPHPOPUP, &layerName)) {
			sceneMgr().nodeRename(node.id(), layerName);
		}
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();
	if (open) {
		if (node.type() == voxel::SceneGraphNodeType::Camera) {
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				video::Camera nodeCamera;
				const voxel::SceneGraphTransform& transform = node.transform();
				nodeCamera.setQuaternion(glm::quat_cast(transform.mat));
				nodeCamera.setWorldPosition(transform.position);
				nodeCamera.setMode(video::CameraMode::Perspective);
				nodeCamera.update(0.0f);
				camera.lerp(nodeCamera);
			}
		} else if (node.type() == voxel::SceneGraphNodeType::Model) {
			const voxel::Region &region = node.region();
			const glm::ivec3 &pos = region.getLowerCorner();
			const glm::ivec3 &size = region.getDimensionsInVoxels();
			ImGui::LabelText(core::string::format("%i:%i:%i", pos.x, pos.y, pos.z).c_str(), "position");
			ImGui::LabelText(core::string::format("%i:%i:%i", size.x, size.y, size.z).c_str(), "size");
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				sceneMgr().nodeActivate(node.id());
			}
		} else if (node.type() == voxel::SceneGraphNodeType::ModelReference) {
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				sceneMgr().nodeActivate(node.referencedNodeId());
			}
		}
		for (const auto& entry : node.properties()) {
			// TODO: allow to edit them
			ImGui::LabelText(entry->value.c_str(), "%s", entry->key.c_str());
		}

		for (int nodeIdx : node.children()) {
			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(nodeIdx), listener);
		}
		ImGui::TreePop();
	}
}

void SceneGraphPanel::update(video::Camera& camera, const char *title, command::CommandExecutionListener &listener) {
	const voxel::SceneGraph &sceneGraph = voxedit::sceneMgr().sceneGraph();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(SceneGraphPanel);
		static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
									   ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
									   ImGuiTableFlags_NoBordersInBody;
		if (ImGui::BeginTable("##scenegraphnodes", 2, flags)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##scenegraphnodeid", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(0), listener);
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

} // namespace voxedit
