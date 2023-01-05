/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "ScopedStyle.h"
#include "Util.h"
#include "core/collection/DynamicArray.h"
#include "core/Color.h"
#include "imgui.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IMGUIEx.h"
#include "voxelformat/SceneGraphNode.h"
#include "DragAndDropPayload.h"
#include <glm/gtc/type_ptr.hpp>

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

static void recursiveAddNodes(video::Camera &camera, const voxelformat::SceneGraph &sceneGraph,
							  voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener,
							  int depth) {
	const int nodeId = node.id();
	bool open = false;

	ImGui::TableNextRow();
	{ // column 1
		ImGui::TableNextColumn();
		const core::String &visibleId = core::string::format("##visible-layer-%i", nodeId);
		bool visible = node.visible();
		if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
			sceneMgr().nodeSetVisible(nodeId, visible);
		}
	}
	{ // column 2
		ImGui::TableNextColumn();
		const core::String &lockedId = core::string::format("##locked-layer-%i", nodeId);
		bool locked = node.locked();
		if (ImGui::Checkbox(lockedId.c_str(), &locked)) {
			sceneMgr().nodeSetLocked(nodeId, locked);
		}
	}
	{ // column 3
		ImGui::TableNextColumn();
		core::RGBA color = node.color();
		glm::vec4 colvec = core::Color::fromRGBA(color);
		const core::String &colorId = core::string::format("Color##layer-%i", nodeId);
		if (ImGui::ColorEdit4(colorId.c_str(), glm::value_ptr(colvec), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
			node.setColor(core::Color::getRGBA(colvec));
		}
	}
	{ // column 4
		ImGui::TableNextColumn();

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
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
		if (node.isLeaf()) {
			treeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		} else {
			treeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		}
		if (selected) {
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		const float indent = (float)depth * (ImGui::GetStyle().FramePadding.x + 4.0f);
		ImGui::Indent(indent);
		if (node.isLeaf()) {
			ImGui::TreeNodeEx(name.c_str(), treeFlags);
		} else {
			open = ImGui::TreeNodeEx(name.c_str(), treeFlags);
		}
		ImGui::Unindent(indent);

		if (node.id() != sceneGraph.root().id()) {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::Text("%s", name.c_str());
				const int sourceNodeId = node.id();
				ImGui::SetDragDropPayload(dragdrop::SceneNodePayload, (const void *)&sourceNodeId, sizeof(int),
										  ImGuiCond_Always);
				ImGui::EndDragDropSource();
			}
		}
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload)) {
				const int sourceNodeId = *(int *)payload->Data;
				const int targetNode = node.id();
				if (!sceneMgr().nodeMove(sourceNodeId, targetNode)) {
					Log::error("Failed to move node");
				}
				ImGui::EndDragDropTarget();
				if (open) {
					ImGui::TreePop();
				}
				return;
				ImGui::EndDragDropTarget();
			}
		}
		contextMenu(camera, sceneGraph, node, listener);
	}
	{ // column 5
		ImGui::TableNextColumn();

		const core::String &deleteId = core::string::format(ICON_FA_TRASH"##delete-layer-%i", nodeId);
		if (ImGui::Button(deleteId.c_str())) {
			sceneMgr().nodeRemove(nodeId, false);
		}
		ImGui::TooltipText("Delete this model");
	}

	if (open) {
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(nodeIdx), listener, depth + 1);
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

void SceneGraphPanel::update(video::Camera& camera, const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener) {
	if (!_animationSpeedVar) {
		_animationSpeedVar = core::Var::getSafe(cfg::VoxEditAnimationSpeed);
	}
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	_hasFocus = false;
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		_hasFocus = ImGui::IsWindowHovered();
		const voxelformat::SceneGraph& sceneGraph = sceneMgr.sceneGraph();
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
		if (ImGui::BeginChild("master##scenegraphpanel", size)) {
			// TODO: Use Toolbar
			// TODO: re-add drag and drop of nodes
			// TODO: cleanup delete nodes/layers
			newLayerButton(sceneGraph, layerSettings);
			ImGui::SameLine();
			newGroupButton(sceneGraph);
			ImGui::SameLine();
			removeNodeButton(sceneGraph);
			ImGui::SameLine();

			const bool onlyOneModel = sceneGraph.size(voxelformat::SceneGraphNodeType::Model) <= 1;
			if (ImGui::DisabledButton(ICON_FA_PLAY"##animatelayers", onlyOneModel)) {
				if (sceneMgr.animateActive()) {
					command::executeCommands("animate 0", &listener);
				} else {
					const core::String& cmd = core::string::format("animate %f", _animationSpeedVar->floatVal());
					command::executeCommands(cmd.c_str(), &listener);
				}
			}
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FA_EYE "##SceneGraphPanel", "layershowall", nullptr, 0, &listener);
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FA_EYE_SLASH "##SceneGraphPanel", "layerhideall", nullptr, 0, &listener);
			if (!onlyOneModel) {
				ImGui::InputVarFloat("Animation speed", _animationSpeedVar);
			}
			static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
												ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
												ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
												ImGuiTableFlags_NoSavedSettings;
			ui::ScopedStyle style;
			style.setIndentSpacing(0.0f);
			if (ImGui::BeginTable("##layerlist", 5, tableFlags)) {
				const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
											ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

				ImGui::TableSetupColumn(ICON_FA_EYE "##visiblelayer", colFlags);
				ImGui::TableSetupColumn(ICON_FA_LOCK "##lockedlayer", colFlags);
				ImGui::TableSetupColumn("##layercolor", colFlags);
				ImGui::TableSetupColumn("Name##layer", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##deletelayer", colFlags);
				ImGui::TableHeadersRow();
				// TODO: filter by name and type
				recursiveAddNodes(camera, sceneGraph, sceneGraph.node(sceneGraph.root().id()), listener, 0);
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		ImGui::Separator();
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		const ImVec2 halfSize(ImGui::GetTextLineHeight() / 2.0f, ImGui::GetTextLineHeight() / 2.0f);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImVec2 trianglePos(pos.x + halfSize.x, pos.y + halfSize.y);
		const ImVec2 buttonSize(halfSize.x * 2.0f, halfSize.y * 2.0f);
		const ImGuiDir buttonDir = _showNodeDetails ? ImGuiDir_Down : ImGuiDir_Up;
		ImGui::RenderArrowPointingAt(drawList, trianglePos, halfSize, buttonDir, ImGui::GetColorU32(ImGuiCol_Text));
		if (ImGui::InvisibleButton("##expandtriangle", buttonSize)) {
			_showNodeDetails ^= true;
			if (_showNodeDetails) {
				ImGui::BeginChild("detail##scenegraphpanel");
				detailView(sceneGraph.node(sceneGraph.activeNode()));
				ImGui::EndChild();
			} else {
				ImGui::TooltipText("Show node details");
			}
		}
	}
	ImGui::End();
}

bool SceneGraphPanel::hasFocus() const {
	return _hasFocus;
}

}
