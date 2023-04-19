/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "Util.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "core/Color.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IMGUIEx.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "scenegraph/SceneGraphNode.h"
#include "DragAndDropPayload.h"
#include <glm/gtc/type_ptr.hpp>

#define SCENEGRAPHPOPUP "##scenegraphpopup"

namespace voxedit {

bool SceneGraphPanel::handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key, const core::String &value) {
	const core::String &id = core::string::format("##%i-%s", node.id(), key.c_str());
	if (key == scenegraph::SceneGraphNodeCamera::PropMode) {
		int currentMode = value == scenegraph::SceneGraphNodeCamera::Modes[0] ? 0 : 1;

		if (ImGui::BeginCombo(id.c_str(), scenegraph::SceneGraphNodeCamera::Modes[currentMode])) {
			for (int n = 0; n < IM_ARRAYSIZE(scenegraph::SceneGraphNodeCamera::Modes); n++) {
				const bool isSelected = (currentMode == n);
				if (ImGui::Selectable(scenegraph::SceneGraphNodeCamera::Modes[n], isSelected)) {
					sceneMgr().nodeSetProperty(node.id(), key, scenegraph::SceneGraphNodeCamera::Modes[n]);
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	} else if (scenegraph::SceneGraphNodeCamera::isFloatProperty(key)) {
		float fvalue = core::string::toFloat(value);
		if (ImGui::InputFloat(id.c_str(), &fvalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			sceneMgr().nodeSetProperty(node.id(), key, core::string::toString(fvalue));
		}
	} else if (scenegraph::SceneGraphNodeCamera::isIntProperty(key)) {
		int ivalue = core::string::toInt(value);
		if (ImGui::InputInt(id.c_str(), &ivalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			sceneMgr().nodeSetProperty(node.id(), key, core::string::toString(ivalue));
		}
	} else {
		return false;
	}
	return true;
}

void SceneGraphPanel::detailView(scenegraph::SceneGraphNode &node) {
	core::String deleteKey;
	static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
										ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
										ImGuiTableFlags_NoSavedSettings;
	ui::ScopedStyle style;
	style.setIndentSpacing(0.0f);
	if (ImGui::BeginTable("##nodelist", 3, tableFlags)) {
		const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
									ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

		ImGui::TableSetupColumn("Name##nodeproperty", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Value##nodeproperty", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##nodepropertydelete", colFlags);
		ImGui::TableHeadersRow();

		for (const auto &entry : node.properties()) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(entry->key.c_str());
			ImGui::TableNextColumn();
			bool propertyAlreadyHandled = false;

			if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
				propertyAlreadyHandled = handleCameraProperty(scenegraph::toCameraNode(node), entry->key, entry->value);
			}

			if (!propertyAlreadyHandled) {
				const core::String &id = core::string::format("##%i-%s", node.id(), entry->key.c_str());
				if (entry->value == "true" || entry->value == "false") {
					bool value = core::string::toBool(entry->value);
					if (ImGui::Checkbox(id.c_str(), &value)) {
						sceneMgr().nodeSetProperty(node.id(), entry->key, value ? "true" : "false");
					}
				} else {
					core::String value = entry->value;
					if (ImGui::InputText(id.c_str(), &value, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
						sceneMgr().nodeSetProperty(node.id(), entry->key, value);
					}
				}
			}
			ImGui::TableNextColumn();
			const core::String &deleteId = core::string::format(ICON_FA_TRASH "##%i-%s-delete", node.id(), entry->key.c_str());
			if (ImGui::Button(deleteId.c_str())) {
				deleteKey = entry->key;
			}
			ImGui::TooltipText("Delete this node property");
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::InputText("##newpropertykey", &_propertyKey);
		ImGui::TableNextColumn();
		ImGui::InputText("##newpropertyvalue", &_propertyValue);
		ImGui::TableNextColumn();
		if (ImGui::Button(ICON_FA_PLUS "##nodepropertyadd")) {
			sceneMgr().nodeSetProperty(node.id(), _propertyKey, _propertyValue);
			_propertyKey = _propertyValue = "";
		}

		ImGui::EndTable();
	}

	if (!deleteKey.empty()) {
		sceneMgr().nodeRemoveProperty(node.id(), deleteKey);
	}
}

static void commandNodeMenu(const char *title, const char *command, const scenegraph::SceneGraphNode &node,
							bool enabled, command::CommandExecutionListener *listener) {
	const core::String &cmd = core::string::format("%s %i", command, node.id());
	ImGui::CommandMenuItem(title, cmd.c_str(), enabled, listener);
}

static void contextMenu(video::Camera& camera, const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	const core::String &contextMenuId = core::string::format("Edit##context-node-%i", node.id());
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		const int validModels = (int)sceneGraph.size();
		commandNodeMenu(ICON_FA_EYE " Show all" SCENEGRAPHPOPUP, "layershowallchildren", node, true, &listener);
		commandNodeMenu(ICON_FA_EYE_SLASH " Hide all" SCENEGRAPHPOPUP, "layerhideallchildren", node, true, &listener);
		commandNodeMenu(ICON_FA_EYE_SLASH " Hide others" SCENEGRAPHPOPUP, "layerhideothers", node, validModels > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_LOCK " Lock all" SCENEGRAPHPOPUP, "layerlockall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_UNLOCK " Unlock all" SCENEGRAPHPOPUP, "layerunlockall", true, &listener);
		commandNodeMenu(ICON_FA_COPY " Duplicate" SCENEGRAPHPOPUP, "nodeduplicate", node, true, &listener);

		if (node.type() == scenegraph::SceneGraphNodeType::Model) {
			commandNodeMenu(ICON_FA_TRASH " Delete" SCENEGRAPHPOPUP, "layerdelete", node, validModels > 1, &listener);
			commandNodeMenu(ICON_FA_COPY " Create reference" SCENEGRAPHPOPUP, "noderef", node, true, &listener);
			const int prevNode = sceneGraph.prevModelNode(node.id());
			commandNodeMenu(ICON_FA_OBJECT_GROUP " Merge" SCENEGRAPHPOPUP, "layermerge", node, prevNode != InvalidNodeId, &listener);
			ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge all" SCENEGRAPHPOPUP, "layermergeall", validModels > 1, &listener);
			ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge visible" SCENEGRAPHPOPUP, "layermergevisible", validModels > 1, &listener);
			ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge locked" SCENEGRAPHPOPUP, "layermergelocked", validModels > 1, &listener);
			ImGui::CommandMenuItem(ICON_FA_DOWN_LEFT_AND_UP_RIGHT_TO_CENTER " Center origin" SCENEGRAPHPOPUP, "center_origin", true, &listener);
			ImGui::CommandMenuItem(ICON_FA_ARROWS_TO_CIRCLE " Center reference" SCENEGRAPHPOPUP, "center_referenceposition", true, &listener);
			commandNodeMenu(ICON_FA_FLOPPY_DISK " Save" SCENEGRAPHPOPUP, "layersave", node, true, &listener);
		} else if (node.type() == scenegraph::SceneGraphNodeType::ModelReference) {
			if (ImGui::MenuItem(ICON_FA_CUBES_STACKED " Convert to model" SCENEGRAPHPOPUP)) {
				sceneMgr().nodeUnreference(node.id());
			}
			ImGui::TooltipText("Unreference from model and allow to edit the voxels for this node");
		}
		ImGui::CommandMenuItem(ICON_FA_FLOPPY_DISK " Save all" SCENEGRAPHPOPUP, "layerssave", true, &listener);

		if (node.type() != scenegraph::SceneGraphNodeType::Model) {
			if (ImGui::MenuItem(ICON_FA_TRASH " Delete" SCENEGRAPHPOPUP)) {
				sceneMgr().nodeRemove(node.id(), true);
			}
			ImGui::TooltipText("Delete this node and all children");
		}
		if (ImGui::MenuItem(ICON_FA_SQUARE_PLUS " Add new group" SCENEGRAPHPOPUP)) {
			scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
			groupNode.setName("new group");
			sceneMgr().addNodeToSceneGraph(groupNode, node.id());
		}
		if (ImGui::MenuItem(ICON_FA_SQUARE_PLUS " Add new camera" SCENEGRAPHPOPUP)) {
			scenegraph::SceneGraphNodeCamera cameraNode = voxelrender::toCameraNode(camera);
			sceneMgr().addNodeToSceneGraph(cameraNode);
		}
		// only on pressing enter to prevent a memento state flood
		if (ImGui::InputText("Name" SCENEGRAPHPOPUP, &node.name(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			sceneMgr().nodeRename(node.id(), node.name());
		}
		ImGui::EndPopup();
	}
}

void SceneGraphPanel::recursiveAddNodes(video::Camera &camera, const scenegraph::SceneGraph &sceneGraph,
							  scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener,
							  int depth, int referencedNodeId) const {
	const int nodeId = node.id();
	bool open = false;
	const bool referenceNode = node.reference() == sceneGraph.activeNode();
	const bool referencedNode = referencedNodeId == nodeId;
	const bool referenceHighlight = referenceNode || referencedNode;

	ImGui::TableNextRow();
	{ // column 1
		ImGui::TableNextColumn();
		const core::String &visibleId = core::string::format("##visible-node-%i", nodeId);
		bool visible = node.visible();
		ui::ScopedStyle style;
		if (_hideInactive->boolVal()) {
			style.disableItem();
		}
		if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
			sceneMgr().nodeSetVisible(nodeId, visible);
		}
		if (_hideInactive->boolVal()) {
			ImGui::TooltipText("Disabled because inactive nodes are hidden and the active node is always visible");
		}
	}
	{ // column 2
		ImGui::TableNextColumn();
		const core::String &lockedId = core::string::format("##locked-node-%i", nodeId);
		bool locked = node.locked();
		if (ImGui::Checkbox(lockedId.c_str(), &locked)) {
			sceneMgr().nodeSetLocked(nodeId, locked);
		}
	}
	{ // column 3
		ImGui::TableNextColumn();
		core::RGBA color = node.color();
		glm::vec4 colvec = core::Color::fromRGBA(color);
		const core::String &colorId = core::string::format("Color##node-%i", nodeId);
		if (ImGui::ColorEdit4(colorId.c_str(), glm::value_ptr(colvec), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
			node.setColor(core::Color::getRGBA(colvec));
		}
	}
	{ // column 4
		ui::ScopedStyle refStyle;
		if (referenceHighlight) {
			refStyle.darker(ImGuiCol_Text);
		}

		ImGui::TableNextColumn();

		core::String name;
		switch (node.type()) {
		case scenegraph::SceneGraphNodeType::ModelReference:
			name = ICON_FA_CUBES_STACKED;
			break;
		case scenegraph::SceneGraphNodeType::Model:
			name = ICON_FA_CUBES;
			break;
		case scenegraph::SceneGraphNodeType::Root:
		case scenegraph::SceneGraphNodeType::Group:
			name = ICON_FA_OBJECT_GROUP;
			break;
		case scenegraph::SceneGraphNodeType::Camera:
			name = ICON_FA_CAMERA;
			break;
		case scenegraph::SceneGraphNodeType::Unknown:
			name = ICON_FA_CIRCLE_QUESTION;
			break;
		case scenegraph::SceneGraphNodeType::AllModels:
		case scenegraph::SceneGraphNodeType::All:
		case scenegraph::SceneGraphNodeType::Max:
			break;
		}
		name.append(core::string::format(" %s##%i", node.name().c_str(), nodeId));
		const bool selected = nodeId == sceneGraph.activeNode();
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

		if (nodeId != sceneGraph.root().id()) {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::Text("%s", name.c_str());
				const int sourceNodeId = nodeId;
				ImGui::SetDragDropPayload(dragdrop::SceneNodePayload, (const void *)&sourceNodeId, sizeof(int),
										  ImGuiCond_Always);
				ImGui::EndDragDropSource();
			}
		}
		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload *payload = ImGui::GetDragDropPayload();
			if (payload->IsDataType(dragdrop::SceneNodePayload)) {
				const int sourceNodeId = *(int *)payload->Data;
				const int targetNode = nodeId;
				if (sceneGraph.canChangeParent(sceneGraph.node(sourceNodeId), targetNode)) {
					if (ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload) != nullptr) {
						if (!sceneMgr().nodeMove(sourceNodeId, targetNode)) {
							Log::error("Failed to move node");
						}
						ImGui::EndDragDropTarget();
						if (open) {
							ImGui::TreePop();
						}
						return;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
		contextMenu(camera, sceneGraph, node, listener);
		if (ImGui::IsItemActivated()) {
			sceneMgr().nodeActivate(nodeId);
		}
		if (referenceNode) {
			ImGui::TooltipText("Reference Node");
		} else if (referencedNode) {
			ImGui::TooltipText("Reference Target Node");
		}
	}
	{ // column 5
		ImGui::TableNextColumn();

		const core::String &deleteId = core::string::format(ICON_FA_TRASH"##delete-node-%i", nodeId);
		if (ImGui::Button(deleteId.c_str())) {
			sceneMgr().nodeRemove(nodeId, false);
		}
		ImGui::TooltipText("Delete this model");
	}

	if (open) {
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(nodeIdx), listener, depth + 1, referencedNodeId);
		}
		ImGui::TreePop();
	}
}

bool SceneGraphPanel::init() {
	_animationSpeedVar = core::Var::getSafe(cfg::VoxEditAnimationSpeed);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	return true;
}

void SceneGraphPanel::update(video::Camera& camera, const char *title, ModelNodeSettings* modelNodeSettings, command::CommandExecutionListener &listener) {
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	_hasFocus = false;

	// TODO handle dragdrop::ModelPayload with the correct parent node

	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		_hasFocus = ImGui::IsWindowHovered();
		const scenegraph::SceneGraph& sceneGraph = sceneMgr.sceneGraph();
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
			const bool onlyOneModel = sceneGraph.size(scenegraph::SceneGraphNodeType::Model) <= 1;
			const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
			ui::Toolbar toolbar(buttonSize);

			toolbar.button(ICON_FA_SQUARE_PLUS, "Add a new model node", [&sceneGraph, this, modelNodeSettings] () {
				const int nodeId = sceneGraph.activeNode();
				scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
				if (node.type() == scenegraph::SceneGraphNodeType::Model) {
					const voxel::RawVolume* v = node.volume();
					const voxel::Region& region = v->region();
					modelNodeSettings->position = region.getLowerCorner();
					modelNodeSettings->size = region.getDimensionsInVoxels();
				}
				if (modelNodeSettings->name.empty()) {
					modelNodeSettings->name = node.name();
				}
				modelNodeSettings->parent = nodeId;
				_popupNewModelNode = true;
			});

			toolbar.button(ICON_FA_SQUARE_PLUS, "Add a new group", [&sceneGraph, &sceneMgr] () {
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
				node.setName("new group");
				sceneMgr.addNodeToSceneGraph(node, sceneGraph.activeNode());
			});

			toolbar.button(ICON_FA_TRASH, "Remove the active node with all its children", [&sceneGraph, &sceneMgr]() {
				sceneMgr.nodeRemove(sceneGraph.activeNode(), true);
			});

			toolbar.custom([&sceneMgr, onlyOneModel, &listener, this, buttonSize] () {
				if (ImGui::DisabledButton(ICON_FA_PLAY, onlyOneModel, buttonSize)) {
					if (sceneMgr.animateActive()) {
						command::executeCommands("animate 0", &listener);
					} else {
						const core::String& cmd = core::string::format("animate %f", _animationSpeedVar->floatVal());
						command::executeCommands(cmd.c_str(), &listener);
					}
				}
				ImGui::TooltipCommand("animate");
			});
			toolbar.button(ICON_FA_EYE, "layershowall");
			toolbar.button(ICON_FA_EYE_SLASH, "layerhideall");
			toolbar.end();
			static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
												ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
												ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
												ImGuiTableFlags_NoSavedSettings;
			ui::ScopedStyle style;
			style.setIndentSpacing(0.0f);
			if (ImGui::BeginTable("##nodelist", 5, tableFlags)) {
				const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
											ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

				ImGui::TableSetupColumn(ICON_FA_EYE "##visiblenode", colFlags);
				ImGui::TableSetupColumn(ICON_FA_LOCK "##lockednode", colFlags);
				ImGui::TableSetupColumn("##nodecolor", colFlags);
				ImGui::TableSetupColumn("Name##node", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##nodedelete", colFlags);
				ImGui::TableHeadersRow();
				// TODO: filter by name and type

				int referencedNode = InvalidNodeId;
				const scenegraph::SceneGraphNode& activeNode = sceneGraph.node(sceneGraph.activeNode());
				if (activeNode.type() == scenegraph::SceneGraphNodeType::ModelReference) {
					referencedNode = activeNode.reference();
				}

				recursiveAddNodes(camera, sceneGraph, sceneGraph.node(sceneGraph.root().id()), listener, 0, referencedNode);
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Details")) {
			_showNodeDetails = true;
			detailView(sceneGraph.node(sceneGraph.activeNode()));
		} else {
			_showNodeDetails = false;
		}
	}
	ImGui::End();
}

bool SceneGraphPanel::hasFocus() const {
	return _hasFocus;
}

}
