/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "DragAndDropPayload.h"
#include "ScopedStyle.h"
#include "ui/imgui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

#define SCENEGRAPHPOPUP "##scenegraphpopup"

namespace voxedit {

static core::String toString(const voxelformat::SceneGraphTransform &transform) {
	core::String str;
	const glm::vec3 &pivot = transform.pivot();
	str.append(core::String::format("piv %.2ff:%.2ff:%.2ff\n", pivot.x, pivot.y, pivot.z));
	const glm::vec3 &tr = transform.worldTranslation();
	str.append(core::String::format("trn %.2ff:%.2ff:%.2ff\n", tr.x, tr.y, tr.z));
	const glm::quat &rt = transform.worldOrientation();
	const glm::vec3 &rtEuler = glm::degrees(glm::eulerAngles(rt));
	str.append(core::String::format("ori %.2ff:%.2ff:%.2ff:%.2ff\n", rt.x, rt.y, rt.z, rt.w));
	str.append(core::String::format("ang %.2ff:%.2ff:%.2ff\n", rtEuler.x, rtEuler.y, rtEuler.z));
	const float sc = transform.worldScale();
	str.append(core::String::format("sca %.2ff\n", sc));
	return str;
}

static void recursiveAddNodes(video::Camera& camera, const voxelformat::SceneGraph &sceneGraph, const voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	ui::imgui::ScopedStyleCompact scopedStyle;
	core::String name;
	switch (node.type()) {
	case voxelformat::SceneGraphNodeType::Model:
		name = ICON_FA_CUBES;
		break;
	case voxelformat::SceneGraphNodeType::Root:
	case voxelformat::SceneGraphNodeType::Group:
		name = ICON_FA_OBJECT_GROUP;
		break;
	case voxelformat::SceneGraphNodeType::Camera:
		name = ICON_FA_CAMERA;
		break;
	case voxelformat::SceneGraphNodeType::Unknown:
		name = ICON_FA_QUESTION_CIRCLE;
		break;
	case voxelformat::SceneGraphNodeType::Max:
		break;
	}
	name.append(core::string::format(" %s##%i", node.name().c_str(), node.id()));
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	const bool selected = node.id() == sceneGraph.activeNode();
	ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
	if (selected) {
		treeFlags |= ImGuiTreeNodeFlags_Selected;
	}
	const bool open = ImGui::TreeNodeEx(name.c_str(), treeFlags);
	if (node.id() != sceneGraph.root().id()) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::Text("%s", name.c_str());
			const int sourceNodeId = node.id();
			ImGui::SetDragDropPayload(dragdrop::SceneNodePayload, (const void*)&sourceNodeId, sizeof(int), ImGuiCond_Always);
			ImGui::EndDragDropSource();
		}
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload)) {
			const int sourceNodeId = *(int*)payload->Data;
			const int targetNode = node.id();
			if (!sceneMgr().nodeMove(sourceNodeId, targetNode)) {
				Log::error("Failed to move node");
			}
			ImGui::EndDragDropTarget();
			ImGui::TableNextColumn();
			if (open) {
				ImGui::TreePop();
			}
			return;
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
		const float maxPropKeyLength = ImGui::CalcTextSize("maxpropertykey").x;
		if (node.type() == voxelformat::SceneGraphNodeType::Camera) {
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				video::Camera nodeCamera(camera);
				const voxelformat::SceneGraphTransform& transform = node.transform();
				nodeCamera.setOrientation(transform.worldOrientation());
				nodeCamera.setWorldPosition(transform.worldTranslation());
				nodeCamera.setMode(video::CameraMode::Perspective);
				nodeCamera.update(0.0f);
				camera.lerp(nodeCamera);
			}
		} else if (node.type() == voxelformat::SceneGraphNodeType::Model) {
			const voxel::Region &region = node.region();
			const glm::ivec3 &pos = region.getLowerCorner();
			const glm::ivec3 &size = region.getDimensionsInVoxels();
			ImGui::PushItemWidth(maxPropKeyLength);
			ImGui::LabelText(core::string::format("%i:%i:%i", pos.x, pos.y, pos.z).c_str(), "position");
			ImGui::LabelText(core::string::format("%i:%i:%i", size.x, size.y, size.z).c_str(), "size");
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				sceneMgr().nodeActivate(node.id());
			}
			ImGui::PopItemWidth();
		}
		ImGui::PushItemWidth(maxPropKeyLength);
		for (const auto& entry : node.properties()) {
			// TODO: allow to edit them
			ImGui::LabelText(entry->value.c_str(), "%s", entry->key.c_str());
		}
		ImGui::PopItemWidth();
		if (node.keyFrames().size() > 1) {
			for (const auto& entry : node.keyFrames()) {
				const core::String &kftText = toString(entry.transform());
				ImGui::TextWrapped("%i (%s, long rotation: %s)\n%s", entry.frameIdx,
								   voxelformat::InterpolationTypeStr[core::enumVal(entry.interpolation)],
								   entry.longRotation ? "true" : "false", kftText.c_str());
			}
		}
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(nodeIdx), listener);
		}
		ImGui::TreePop();
	}
}

void SceneGraphPanel::update(video::Camera& camera, const char *title, command::CommandExecutionListener &listener) {
	const voxelformat::SceneGraph &sceneGraph = voxedit::sceneMgr().sceneGraph();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(SceneGraphPanel);
		static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
									   ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
									   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
									   ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit;
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
