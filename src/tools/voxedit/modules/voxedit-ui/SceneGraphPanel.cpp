/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "ui/IconsLucide.h"
#include "core/Optional.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IMGUIEx.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "scenegraph/SceneGraphNode.h"
#include "DragAndDropPayload.h"
#include "voxelrender/SceneGraphRenderer.h"
#include <glm/gtc/type_ptr.hpp>

#define SCENEGRAPHPOPUP "##scenegraphpopup"
#define SCENEGRAPHDRAGANDDROPPOPUP "##scenegraphdraganddroppopup"

namespace voxedit {

bool SceneGraphPanel::handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key, const core::String &value) {
	const core::String &id = core::string::format("##%i-%s", node.id(), key.c_str());
	if (key == scenegraph::SceneGraphNodeCamera::PropMode) {
		int currentMode = value == scenegraph::SceneGraphNodeCamera::Modes[0] ? 0 : 1;

		if (ImGui::BeginCombo(id.c_str(), scenegraph::SceneGraphNodeCamera::Modes[currentMode])) {
			for (int n = 0; n < IM_ARRAYSIZE(scenegraph::SceneGraphNodeCamera::Modes); n++) {
				const bool isSelected = (currentMode == n);
				if (ImGui::Selectable(scenegraph::SceneGraphNodeCamera::Modes[n], isSelected)) {
					_sceneMgr->nodeSetProperty(node.id(), key, scenegraph::SceneGraphNodeCamera::Modes[n]);
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
			_sceneMgr->nodeSetProperty(node.id(), key, core::string::toString(fvalue));
		}
	} else if (scenegraph::SceneGraphNodeCamera::isIntProperty(key)) {
		int ivalue = core::string::toInt(value);
		if (ImGui::InputInt(id.c_str(), &ivalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			_sceneMgr->nodeSetProperty(node.id(), key, core::string::toString(ivalue));
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

		// TODO: use ImGuiListClipper
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
						_sceneMgr->nodeSetProperty(node.id(), entry->key, value ? "true" : "false");
					}
				} else {
					core::String value = entry->value;
					if (ImGui::InputText(id.c_str(), &value, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
						_sceneMgr->nodeSetProperty(node.id(), entry->key, value);
					}
				}
			}
			ImGui::TableNextColumn();
			const core::String &deleteId = core::string::format(ICON_LC_TRASH "##%i-%s-delete", node.id(), entry->key.c_str());
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
		if (ImGui::Button(ICON_LC_PLUS "##nodepropertyadd")) {
			_sceneMgr->nodeSetProperty(node.id(), _propertyKey, _propertyValue);
			_propertyKey = _propertyValue = "";
		}

		ImGui::EndTable();
	}

	if (!deleteKey.empty()) {
		_sceneMgr->nodeRemoveProperty(node.id(), deleteKey);
	}
}

static void commandNodeMenu(const char *icon, const char *title, const char *command, int nodeId,
							bool enabled, command::CommandExecutionListener *listener) {
	const core::String &cmd = core::string::format("%s %i", command, nodeId);
	ImGui::CommandIconMenuItem(icon, title, cmd.c_str(), enabled, listener);
}

void SceneGraphPanel::contextMenu(video::Camera& camera, const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	const int nodeId = node.id();
	const core::String &contextMenuId = core::string::format("Edit##context-node-%i", nodeId);
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		const int validModels = (int)sceneGraph.size();
		scenegraph::SceneGraphNodeType nodeType = node.type();

		// don't access node data below this - the commands that are executed here can make the node reference invalid

		ImGui::CommandIconMenuItem(ICON_LC_TERMINAL, _("Rename" SCENEGRAPHPOPUP), "toggle ve_popuprenamenode", true, &listener);
		commandNodeMenu(ICON_LC_EYE, _("Show all" SCENEGRAPHPOPUP), "nodeshowallchildren", nodeId, true, &listener);
		commandNodeMenu(ICON_LC_EYE_OFF, _("Hide all" SCENEGRAPHPOPUP), "nodehideallchildren", nodeId, true, &listener);
		commandNodeMenu(ICON_LC_EYE_OFF, _("Hide others" SCENEGRAPHPOPUP), "nodehideothers", nodeId, validModels > 1, &listener);
		ImGui::CommandIconMenuItem(ICON_LC_LOCK, _("Lock all" SCENEGRAPHPOPUP), "modellockall", true, &listener);
		ImGui::CommandIconMenuItem(ICON_LC_UNLOCK, _("Unlock all" SCENEGRAPHPOPUP), "modelunlockall", true, &listener);
		commandNodeMenu(ICON_LC_COPY, _("Duplicate" SCENEGRAPHPOPUP), "nodeduplicate", nodeId, true, &listener);
		commandNodeMenu(ICON_LC_TRASH, _("Delete" SCENEGRAPHPOPUP), "nodedelete", nodeId, true, &listener);

		if (nodeType == scenegraph::SceneGraphNodeType::Model) {
			commandNodeMenu(ICON_LC_COPY, _("Create reference" SCENEGRAPHPOPUP), "modelref", nodeId, true, &listener);
			const int prevNode = sceneGraph.prevModelNode(nodeId);
			commandNodeMenu(ICON_LC_GROUP, _("Merge" SCENEGRAPHPOPUP), "modelmerge", nodeId, prevNode != InvalidNodeId, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_GROUP, _("Merge all" SCENEGRAPHPOPUP), "modelmergeall", validModels > 1, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_GROUP, _("Merge visible" SCENEGRAPHPOPUP), "modelmergevisible", validModels > 1, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_GROUP, _("Merge locked" SCENEGRAPHPOPUP), "modelmergelocked", validModels > 1, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SHRINK, _("Center origin" SCENEGRAPHPOPUP), "center_origin", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SHRINK, _("Center reference" SCENEGRAPHPOPUP), "center_referenceposition", true, &listener);
			commandNodeMenu(ICON_LC_SAVE, _("Save" SCENEGRAPHPOPUP), "modelsave", nodeId, true, &listener);
		} else if (nodeType == scenegraph::SceneGraphNodeType::ModelReference) {
			ImGui::CommandIconMenuItem(ICON_LC_CODESANDBOX, _("Convert to model" SCENEGRAPHPOPUP), "modelunref", true, &listener);
		}
		ImGui::CommandIconMenuItem(ICON_LC_SAVE, _("Save all" SCENEGRAPHPOPUP), "modelssave", validModels > 1, &listener);

		if (ImGui::IconMenuItem(ICON_LC_PLUS_SQUARE, _("Add new group" SCENEGRAPHPOPUP))) {
			scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
			groupNode.setName("new group");
			_sceneMgr->moveNodeToSceneGraph(groupNode, nodeId);
		}
		if (ImGui::IconMenuItem(ICON_LC_PLUS_SQUARE, _("Add new camera" SCENEGRAPHPOPUP))) {
			scenegraph::SceneGraphNodeCamera cameraNode = voxelrender::toCameraNode(camera);
			_sceneMgr->moveNodeToSceneGraph(cameraNode);
		}
		ImGui::EndPopup();
	}
}

void SceneGraphPanel::recursiveAddNodes(video::Camera &camera, const scenegraph::SceneGraph &sceneGraph,
							  scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener,
							  int depth, int referencedNodeId) {
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
			_sceneMgr->nodeSetVisible(nodeId, visible);
		}
		if (_hideInactive->boolVal()) {
			ImGui::TooltipText(_("Disabled because inactive nodes are hidden and the active node is always visible"));
		}
	}
	{ // column 2
		ImGui::TableNextColumn();
		const core::String &lockedId = core::string::format("##locked-node-%i", nodeId);
		bool locked = node.locked();
		if (ImGui::Checkbox(lockedId.c_str(), &locked)) {
			_sceneMgr->nodeSetLocked(nodeId, locked);
		}
	}
	{ // column 3
		ImGui::TableNextColumn();
		core::RGBA color = node.color();
		glm::vec4 colvec = core::Color::fromRGBA(color);
		const core::String &colorId = core::string::format(_("Color##node-%i"), nodeId);
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
			name = ICON_LC_CODESANDBOX;
			break;
		case scenegraph::SceneGraphNodeType::Model:
			name = ICON_LC_BOXES;
			break;
		case scenegraph::SceneGraphNodeType::Root:
		case scenegraph::SceneGraphNodeType::Group:
			name = ICON_LC_GROUP;
			break;
		case scenegraph::SceneGraphNodeType::Camera:
			name = ICON_LC_CAMERA;
			break;
		case scenegraph::SceneGraphNodeType::Unknown:
			name = ICON_LC_HELP_CIRCLE;
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
				ImGui::TextUnformatted(name.c_str());
				const int sourceNodeId = nodeId;
				ImGui::SetDragDropPayload(dragdrop::SceneNodePayload, (const void *)&sourceNodeId, sizeof(int),
										  ImGuiCond_Always);
				ImGui::EndDragDropSource();
			}
		}
		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload *payload = ImGui::GetDragDropPayload();
			if (payload->IsDataType(dragdrop::SceneNodePayload)) {
				if (ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload) != nullptr) {
					_dragDropSourceNodeId = *(int *)payload->Data;
					_dragDropTargetNodeId = nodeId;
					_popupDragAndDrop = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		contextMenu(camera, sceneGraph, node, listener);
		if (ImGui::IsItemActivated()) {
			_sceneMgr->nodeActivate(nodeId);
		}
		if (referenceNode) {
			ImGui::TooltipText(_("Reference Node"));
		} else if (referencedNode) {
			ImGui::TooltipText(_("Reference Target Node"));
		}
	}
	{ // column 5
		ImGui::TableNextColumn();

		const core::String &deleteId = core::string::format(ICON_LC_TRASH"##delete-node-%i", nodeId);
		if (ImGui::Button(deleteId.c_str())) {
			_sceneMgr->nodeRemove(nodeId, false);
		}
		ImGui::TooltipText(_("Delete this model"));
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
	_hasFocus = false;

	// TODO handle dragdrop::ModelPayload with the correct parent node

	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		_hasFocus = ImGui::IsWindowHovered();
		const scenegraph::SceneGraph& sceneGraph = _sceneMgr->sceneGraph();
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

			toolbar.button(ICON_LC_PLUS_SQUARE, _("Add a new model node"), [&sceneGraph, this, modelNodeSettings] () {
				const int nodeId = sceneGraph.activeNode();
				modelNodeSettings->palette.setValue(nullptr);
				scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
				if (node.isModelNode()) {
					const voxel::RawVolume* v = node.volume();
					const voxel::Region& region = v->region();
					modelNodeSettings->position = region.getLowerCorner();
					modelNodeSettings->size = region.getDimensionsInVoxels();
					modelNodeSettings->palette.setValue(node.palette());
				}
				if (modelNodeSettings->name.empty()) {
					modelNodeSettings->name = node.name();
				}
				modelNodeSettings->parent = nodeId;
				_popupNewModelNode = true;
			});

			toolbar.button(ICON_LC_GROUP, _("Add a new group"), [&sceneGraph, this] () {
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
				node.setName("new group");
				_sceneMgr->moveNodeToSceneGraph(node, sceneGraph.activeNode());
			});

			toolbar.button(ICON_LC_TRASH, _("Remove the active node with all its children"), [&sceneGraph, this]() {
				_sceneMgr->nodeRemove(sceneGraph.activeNode(), true);
			});

			toolbar.custom([onlyOneModel, &listener, this, buttonSize] () {
				if (ImGui::DisabledButton(ICON_LC_PLAY, onlyOneModel, buttonSize)) {
					if (_sceneMgr->animateActive()) {
						command::executeCommands("animate 0", &listener);
					} else {
						const core::String& cmd = core::string::format("animate %f", _animationSpeedVar->floatVal());
						command::executeCommands(cmd.c_str(), &listener);
					}
				}
				ImGui::TooltipCommand("animate");
			});
			toolbar.button(ICON_LC_EYE, "showall");
			toolbar.button(ICON_LC_EYE_OFF, "hideall");
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

				ImGui::TableSetupColumn(ICON_LC_EYE "##visiblenode", colFlags);
				ImGui::TableSetupColumn(ICON_LC_LOCK "##lockednode", colFlags);
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
		if (ImGui::CollapsingHeader(_("Details"))) {
			_showNodeDetails = true;
			detailView(sceneGraph.node(sceneGraph.activeNode()));
		} else {
			_showNodeDetails = false;
		}
	}
	ImGui::End();

	if (_popupDragAndDrop) {
		ImGui::OpenPopup(SCENEGRAPHDRAGANDDROPPOPUP);
		_popupDragAndDrop = false;
	}

	registerPopups();
}

void SceneGraphPanel::registerPopups() {
	ImGuiWindowFlags popupFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginPopup(SCENEGRAPHDRAGANDDROPPOPUP, popupFlags)) {
		const scenegraph::SceneGraph& sceneGraph = _sceneMgr->sceneGraph();
		const scenegraph::SceneGraphNode *sourceNode = _sceneMgr->sceneGraphNode(_dragDropSourceNodeId);
		const scenegraph::SceneGraphNode *targetNode = _sceneMgr->sceneGraphNode(_dragDropTargetNodeId);

		const bool canChangeParent = sceneGraph.canChangeParent(sceneGraph.node(_dragDropSourceNodeId), _dragDropTargetNodeId);
		if (sourceNode && targetNode) {
			if (sourceNode->isModelNode() && targetNode->isModelNode()) {
				if (ImGui::IconButton(ICON_LC_LINK, _("Merge onto##mergeonto"))) {
					_sceneMgr->mergeNodes(_dragDropTargetNodeId, _dragDropSourceNodeId);
					ImGui::CloseCurrentPopup();
				}
				ImGui::TooltipText(_("Merge %s onto %s"), sourceNode->name().c_str(), targetNode->name().c_str());
			}
		}
		if (canChangeParent) {
			if (ImGui::IconButton(ICON_LC_INDENT, _("Move below"))) {
				if (!_sceneMgr->nodeMove(_dragDropSourceNodeId, _dragDropTargetNodeId)) {
					Log::error("Failed to move node");
				}
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
}

bool SceneGraphPanel::hasFocus() const {
	return _hasFocus;
}

}
