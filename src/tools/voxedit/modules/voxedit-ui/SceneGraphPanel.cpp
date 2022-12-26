/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "DragAndDropPayload.h"
#include "imgui.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/ScopedStyle.h"
#include "ui/IMGUIEx.h"
#include "video/Camera.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelrender/SceneGraphRenderer.h"

#define SCENEGRAPHPOPUP "##scenegraphpopup"

namespace voxedit {

static core::String toString(const voxelformat::SceneGraphTransform &transform) {
	core::String str;
	const glm::vec3 &pivot = transform.pivot();
	str.append(core::String::format("piv %.2f:%.2f:%.2f\n", pivot.x, pivot.y, pivot.z));
	const glm::vec3 &tr = transform.worldTranslation();
	str.append(core::String::format("trn %.2f:%.2f:%.2f\n", tr.x, tr.y, tr.z));
	const glm::quat &rt = transform.worldOrientation();
	const glm::vec3 &rtEuler = glm::degrees(glm::eulerAngles(rt));
	str.append(core::String::format("ori %.2ff%.2f:%.2f:%.2f\n", rt.x, rt.y, rt.z, rt.w));
	str.append(core::String::format("ang %.2f:%.2f:%.2f\n", rtEuler.x, rtEuler.y, rtEuler.z));
	const glm::vec3 &sc = transform.worldScale();
	str.append(core::String::format("sca %.2f:%.2f:%.2f\n", sc.x, sc.y, sc.z));
	return str;
}

static void detailView(const voxelformat::SceneGraphNode &node) {
	const float maxPropKeyLength = ImGui::CalcTextSize("maxpropertykey").x;
	if (node.type() == voxelformat::SceneGraphNodeType::Model) {
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
	for (const auto &entry : node.properties()) {
		// TODO: allow to edit them
		ImGui::LabelText(entry->value.c_str(), "%s", entry->key.c_str());
	}
	ImGui::PopItemWidth();
	if (node.keyFrames().size() > 1) {
		for (const auto &entry : node.keyFrames()) {
			const core::String &kftText = toString(entry.transform());
			ImGui::TextWrapped("%i (%s, long rotation: %s)\n%s", entry.frameIdx,
							voxelformat::InterpolationTypeStr[core::enumVal(entry.interpolation)],
							entry.longRotation ? "true" : "false", kftText.c_str());
		}
	}
}

static void contextMenu(video::Camera& camera, const voxelformat::SceneGraph &sceneGraph, const voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	const core::String &contextMenuId = core::string::format("Edit##context-node-%i", node.id());
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		const int validLayers = (int)sceneGraph.size();
		if (node.type() == voxelformat::SceneGraphNodeType::Model) {
			ImGui::CommandMenuItem(ICON_FA_TRASH " Delete" SCENEGRAPHPOPUP, "layerdelete", validLayers > 1, &listener);
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
			ImGui::CommandMenuItem(ICON_FA_DOWN_LEFT_AND_UP_RIGHT_TO_CENTER " Center origin" SCENEGRAPHPOPUP, "center_origin", true, &listener);
			ImGui::CommandMenuItem(ICON_FA_ARROWS_TO_CIRCLE " Center reference" SCENEGRAPHPOPUP, "center_referenceposition", true, &listener);
			ImGui::CommandMenuItem(ICON_FA_FLOPPY_DISK " Save" SCENEGRAPHPOPUP, "layerssave", true, &listener);
		} else {
			if (ImGui::MenuItem(ICON_FA_TRASH " Delete" SCENEGRAPHPOPUP)) {
				sceneMgr().nodeRemove(node.id(), true);
			}
			ImGui::TooltipText("Delete this node and all children");
		}
		if (ImGui::MenuItem(ICON_FA_SQUARE_PLUS " Add new group" SCENEGRAPHPOPUP)) {
			voxelformat::SceneGraphNode groupNode(voxelformat::SceneGraphNodeType::Group);
			groupNode.setName("new group");
			sceneMgr().addNodeToSceneGraph(groupNode, node.id());
		}
		if (ImGui::MenuItem(ICON_FA_SQUARE_PLUS " Add new camera" SCENEGRAPHPOPUP)) {
			voxelformat::SceneGraphNodeCamera cameraNode;
			voxelformat::SceneGraphTransform transform;
			const voxelformat::KeyFrameIndex keyFrameIdx = 0;
			transform.setWorldMatrix(camera.viewMatrix());
			cameraNode.setTransform(keyFrameIdx, transform);
			cameraNode.setFarPlane(camera.farPlane());
			cameraNode.setNearPlane(camera.nearPlane());
			if (camera.mode() == video::CameraMode::Orthogonal) {
				cameraNode.setOrthographic();
			} else {
				cameraNode.setPerspective();
			}
			cameraNode.setFieldOfView((int)camera.fieldOfView());
			cameraNode.setName("new camera");
			sceneMgr().addNodeToSceneGraph(cameraNode);
		}
		core::String layerName = node.name();
		if (ImGui::InputText("Name" SCENEGRAPHPOPUP, &layerName)) {
			sceneMgr().nodeRename(node.id(), layerName);
		}
		ImGui::EndPopup();
	}
}

static void recursiveAddNodes(video::Camera& camera, const voxelformat::SceneGraph &sceneGraph, const voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	ui::ScopedStyleCompact scopedStyle;
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
		name = ICON_FA_CIRCLE_QUESTION;
		break;
	case voxelformat::SceneGraphNodeType::Max:
		break;
	}
	name.append(core::string::format(" %s##%i", node.name().c_str(), node.id()));
	const bool selected = node.id() == sceneGraph.activeNode();
	ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
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
			if (open) {
				ImGui::TreePop();
			}
			return;
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::IsItemActivated()) {
		sceneMgr().nodeActivate(node.id());
	}

	contextMenu(camera, sceneGraph, node, listener);

	if (open) {
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(nodeIdx), listener);
		}
		ImGui::TreePop();
	}
}

void SceneGraphPanel::newLayerButton(const voxelformat::SceneGraph &sceneGraph, LayerSettings* layerSettings) {
	if (ImGui::Button(ICON_FA_SQUARE_PLUS"##newlayer")) {
		const int nodeId = sceneGraph.activeNode();
		voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
		const voxel::RawVolume* v = node.volume();
		const voxel::Region& region = v->region();
		layerSettings->position = region.getLowerCorner();
		layerSettings->size = region.getDimensionsInVoxels();
		if (layerSettings->name.empty()) {
			layerSettings->name = node.name();
		}
		layerSettings->parent = nodeId;
		_popupNewLayer = true;
	}
	ImGui::TooltipText("Add a new model node");
}

static void newGroupButton(const voxelformat::SceneGraph &sceneGraph) {
	if (ImGui::Button(ICON_FA_SQUARE_PLUS "##scenegraphnewgroup")) {
		voxelformat::SceneGraphNode node(voxelformat::SceneGraphNodeType::Group);
		node.setName("new group");
		sceneMgr().addNodeToSceneGraph(node, sceneGraph.activeNode());
	}
	ImGui::TooltipText("Add a new group");
}

static void removeNodeButton(const voxelformat::SceneGraph &sceneGraph) {
	if (ImGui::Button(ICON_FA_TRASH "##scenegraphremovenode")) {
		sceneMgr().nodeRemove(sceneGraph.activeNode(), true);
	}
	ImGui::TooltipText("Remove the active node with all its children");
}

void SceneGraphPanel::update(video::Camera &camera, const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener) {
	const voxelformat::SceneGraph &sceneGraph = voxedit::sceneMgr().sceneGraph();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(SceneGraphPanel);
		ImVec2 size = ImGui::GetWindowSize();
		const float textLineHeight = ImGui::GetTextLineHeight();
		if (_showNodeDetails) {
			size.y -= textLineHeight * 10.0f;
		} else {
			size.y -= textLineHeight * 4.0f;
		}
		if (size.y <= textLineHeight * 2.0f) {
			size.y = textLineHeight * 2.0f;
		}
		ImGui::BeginChild("master##scenegraphpanel", size);

		newLayerButton(sceneGraph, layerSettings);
		ImGui::SameLine();
		newGroupButton(sceneGraph);
		ImGui::SameLine();
		removeNodeButton(sceneGraph);

		// TODO: filter by name and type
		recursiveAddNodes(camera, sceneGraph, sceneGraph.node(sceneGraph.root().id()), listener);
		ImGui::EndChild();

		ImGui::Separator();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 halfSize(ImGui::GetTextLineHeight() / 2.0f, ImGui::GetTextLineHeight() / 2.0f);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImVec2 trianglePos(pos.x + halfSize.x, pos.y + halfSize.y);
		const ImVec2 buttonSize(halfSize.x * 2.0f, halfSize.y * 2.0f);
		const ImGuiDir buttonDir = _showNodeDetails ? ImGuiDir_Down : ImGuiDir_Up;
		ImGui::RenderArrowPointingAt(drawList, trianglePos, halfSize, buttonDir, ImGui::GetColorU32(ImGuiCol_Text));
		if (ImGui::InvisibleButton("##expandtriangle", buttonSize)) {
			_showNodeDetails ^= true;
			Log::error("toggle: %s", _showNodeDetails ? "true" : "false");
		}
		if (_showNodeDetails) {
			ImGui::BeginChild("detail##scenegraphpanel");
			detailView(sceneGraph.node(sceneGraph.activeNode()));
			ImGui::EndChild();
		} else {
			ImGui::TooltipText("Show node details");
		}
	}
	ImGui::End();
}

} // namespace voxedit
