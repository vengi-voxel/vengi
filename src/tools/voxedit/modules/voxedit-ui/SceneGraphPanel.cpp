/**
 * @file
 */

#include "SceneGraphPanel.h"
#include "DragAndDropPayload.h"
#include "command/CommandHandler.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/Optional.h"
#include "core/StringUtil.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedStyle.h"
#include "ui/Toolbar.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/SceneManager.h"
#include "voxelrender/SceneGraphRenderer.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

static void commandNodeMenu(const char *icon, const char *title, const char *command, int nodeId,
							bool enabled, command::CommandExecutionListener *listener) {
	char cmd[64];
	core::String::formatBuf(cmd, sizeof(cmd), "%s %i", command, nodeId);
	ImGui::CommandIconMenuItem(icon, title, cmd, enabled, listener);
}

void SceneGraphPanel::contextMenu(video::Camera& camera, const scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	const int nodeId = node.id();
	char contextMenuId[64];
	core::String::formatBuf(contextMenuId, sizeof(contextMenuId), "Edit##context-node-%i", nodeId);
	if (ImGui::BeginPopupContextItem(contextMenuId)) {
		const int validModels = (int)sceneGraph.size();
		scenegraph::SceneGraphNodeType nodeType = node.type();

		// don't access node data below this - the commands that are executed here can make the node reference invalid

		ImGui::CommandIconMenuItem(ICON_LC_TERMINAL, _("Rename"), "toggle ve_popuprenamenode", true, &listener);
		commandNodeMenu(ICON_LC_EYE, _("Show all"), "nodeshowallchildren", nodeId, true, &listener);
		commandNodeMenu(ICON_LC_EYE_OFF, _("Hide all"), "nodehideallchildren", nodeId, true, &listener);
		commandNodeMenu(ICON_LC_EYE_OFF, _("Hide others"), "nodehideothers", nodeId, validModels > 1, &listener);
		ImGui::CommandIconMenuItem(ICON_LC_LOCK, _("Lock all"), "modellockall", true, &listener);
		ImGui::CommandIconMenuItem(ICON_LC_LOCK_OPEN, _("Unlock all"), "modelunlockall", true, &listener);
		commandNodeMenu(ICON_LC_COPY, _("Duplicate"), "nodeduplicate", nodeId, true, &listener);
		commandNodeMenu(ICON_LC_TRASH, _("Delete"), "nodedelete", nodeId, true, &listener);

		if (nodeType == scenegraph::SceneGraphNodeType::Model) {
			commandNodeMenu(ICON_LC_COPY, _("Create reference"), "modelref", nodeId, true, &listener);
			const int prevNode = sceneGraph.prevModelNode(nodeId);
			commandNodeMenu(ICON_LC_GROUP, _("Merge"), "modelmerge", nodeId, prevNode != InvalidNodeId, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_COPY, _("Use as stamp"), "stampbrushusenode", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_GROUP, _("Merge all"), "modelmergeall", validModels > 1, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_GROUP, _("Merge visible"), "modelmergevisible", validModels > 1, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_GROUP, _("Merge locked"), "modelmergelocked", validModels > 1, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SHRINK, _("Center origin"), "center_origin", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SHRINK, _("Center reference"), "center_referenceposition", true, &listener);
			commandNodeMenu(ICON_LC_SAVE, _("Save"), "modelsave", nodeId, true, &listener);
		} else if (nodeType == scenegraph::SceneGraphNodeType::ModelReference) {
			ImGui::CommandIconMenuItem(ICON_LC_CODESANDBOX, _("Convert to model"), "modelunref", true, &listener);
		} else if (nodeType == scenegraph::SceneGraphNodeType::Camera) {
			commandNodeMenu(ICON_LC_CAMERA, _("Use this camera"), "cam_activate", nodeId, true, &listener);
		}
		ImGui::CommandIconMenuItem(ICON_LC_SAVE, _("Save all"), "modelssave", validModels > 1, &listener);

		if (ImGui::IconMenuItem(ICON_LC_SQUARE_PLUS, _("Add new group"))) {
			scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
			groupNode.setName("new group");
			_sceneMgr->moveNodeToSceneGraph(groupNode, nodeId);
		}
		if (ImGui::IconMenuItem(ICON_LC_SQUARE_PLUS, _("Add new camera"))) {
			scenegraph::SceneGraphNodeCamera cameraNode = voxelrender::toCameraNode(camera);
			_sceneMgr->moveNodeToSceneGraph(cameraNode);
		}
		if (ImGui::IconMenuItem(ICON_LC_SQUARE_PLUS, _("Add new point"))) {
			scenegraph::SceneGraphNode pointNode(scenegraph::SceneGraphNodeType::Point);
			pointNode.setName("new point");
			scenegraph::SceneGraphTransform transform;
			transform.setLocalTranslation(_sceneMgr->referencePosition());
			scenegraph::KeyFrameIndex keyFrameIdx = 0;
			pointNode.setTransform(keyFrameIdx, transform);
			_sceneMgr->moveNodeToSceneGraph(pointNode, nodeId);
		}
		ImGui::EndPopup();
	}
}

// see filterTypes array
bool SceneGraphPanel::isFiltered(const scenegraph::SceneGraphNode &node) const {
	if (_filterType != 0) {
		if (_filterType == 1 && !node.isModelNode()) {
			return true;
		}
		if (_filterType == 2 && !node.isGroupNode()) {
			return true;
		}
		if (_filterType == 3 && !node.isCameraNode()) {
			return true;
		}
		if (_filterType == 4 && !node.isReferenceNode()) {
			return true;
		}
		if (_filterType == 5 && !node.isPointNode()) {
			return true;
		}
	}
	if (!_filterName.empty() && !core::string::icontains(node.name(), _filterName)) {
		return true;
	}

	return false;
}

void SceneGraphPanel::recursiveAddNodes(video::Camera &camera, const scenegraph::SceneGraph &sceneGraph,
							  scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener,
							  int depth, int referencedNodeId) {
	core_trace_scoped(RecursiveAddNodes);
	const int nodeId = node.id();
	bool open = false;
	const int activeNode = sceneGraph.activeNode();
	const bool referenceNode = node.reference() == activeNode;
	const bool referencedNode = referencedNodeId == nodeId;
	const bool referenceHighlight = referenceNode || referencedNode;

	const bool filtered = isFiltered(node);

	if (!filtered) {
		ImGui::TableNextRow();
		char idbuf[64];
		{ // column 1
			ImGui::TableNextColumn();
			core::String::formatBuf(idbuf, sizeof(idbuf), "##visible-node-%i", nodeId);
			bool visible = node.visible();
			{
				ui::ScopedStyle style;
				ImGui::BeginDisabled(_hideInactive->boolVal());
				if (ImGui::Checkbox(idbuf, &visible)) {
					command::executeCommands("nodetogglevisible " + core::string::toString(nodeId), &listener);
				}
				ImGui::EndDisabled();
			}
			if (_hideInactive->boolVal()) {
				ImGui::TooltipTextUnformatted(_("Disabled because inactive nodes are hidden and the active node is always visible"));
			}
		}
		{ // column 2
			ImGui::TableNextColumn();
			core::String::formatBuf(idbuf, sizeof(idbuf), "##locked-node-%i", nodeId);
			bool locked = node.locked();
			if (ImGui::Checkbox(idbuf, &locked)) {
				command::executeCommands("nodetogglelock " + core::string::toString(nodeId), &listener);
			}
		}
		{ // column 3
			ImGui::TableNextColumn();
			core::RGBA color = node.color();
			glm::vec4 colvec = core::Color::fromRGBA(color);
			core::String::formatBuf(idbuf, sizeof(idbuf), _("Color##node-%i"), nodeId);
			if (ImGui::ColorEdit4(idbuf, glm::value_ptr(colvec), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
				node.setColor(core::Color::getRGBA(colvec));
			}
		}
		{ // column 4
			ui::ScopedStyle refStyle;
			if (referenceHighlight) {
				refStyle.darker(ImGuiCol_Text);
			}

			ImGui::TableNextColumn();

			const char *icon = "";
			switch (node.type()) {
			case scenegraph::SceneGraphNodeType::ModelReference:
				icon = ICON_LC_CODESANDBOX;
				break;
			case scenegraph::SceneGraphNodeType::Model:
				icon = ICON_LC_BOXES;
				break;
			case scenegraph::SceneGraphNodeType::Point:
				icon = ICON_LC_POINTER;
				break;
			case scenegraph::SceneGraphNodeType::Root:
			case scenegraph::SceneGraphNodeType::Group:
				icon = ICON_LC_GROUP;
				break;
			case scenegraph::SceneGraphNodeType::Camera:
				icon = ICON_LC_CAMERA;
				break;
			case scenegraph::SceneGraphNodeType::Unknown:
				icon = ICON_LC_CIRCLE_QUESTION_MARK;
				break;
			case scenegraph::SceneGraphNodeType::AllModels:
			case scenegraph::SceneGraphNodeType::All:
			case scenegraph::SceneGraphNodeType::Max:
				break;
			}
			const core::String &name = core::String::format("%s##%i", node.name().c_str(), nodeId);
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
				ImGui::IconTreeNodeEx(icon, name.c_str(), treeFlags);
			} else {
				open = ImGui::IconTreeNodeEx(icon, name.c_str(), treeFlags);
			}
			ImGui::Unindent(indent);

			if (activeNode != _lastActivedNodeId && nodeId == activeNode) {
				ImGui::SetScrollHereY();
				_lastActivedNodeId = activeNode;
			}

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
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload)) {
					_dragDropSourceNodeId = *(int *)payload->Data;
					_dragDropTargetNodeId = nodeId;
					_popupDragAndDrop = true;
				}
				ImGui::EndDragDropTarget();
			}
			contextMenu(camera, sceneGraph, node, listener);
			if (ImGui::IsItemActivated()) {
				_sceneMgr->nodeActivate(nodeId);
				_lastActivedNodeId = nodeId;
			}
			if (referenceNode) {
				ImGui::TooltipTextUnformatted(_("Reference Node"));
			} else if (referencedNode) {
				ImGui::TooltipTextUnformatted(_("Reference Target Node"));
			}
		}
		{ // column 5
			ImGui::TableNextColumn();

			core::String::formatBuf(idbuf, sizeof(idbuf), ICON_LC_TRASH"##delete-node-%i", nodeId);
			if (ImGui::Button(idbuf)) {
				_sceneMgr->nodeRemove(nodeId, false);
			}
			ImGui::TooltipTextUnformatted(_("Delete this model"));
		}
	}

	if (open || filtered) {
		for (int nodeIdx : node.children()) {
			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(nodeIdx), listener, depth + 1, referencedNodeId);
		}
		if (open) {
			ImGui::TreePop();
		}
	}
}

bool SceneGraphPanel::init() {
	_animationSpeedVar = core::Var::getSafe(cfg::VoxEditAnimationSpeed);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	const scenegraph::SceneGraph& sceneGraph = _sceneMgr->sceneGraph();
	_lastActivedNodeId = sceneGraph.activeNode();
	return true;
}

void SceneGraphPanel::update(video::Camera& camera, const char *id, ModelNodeSettings* modelNodeSettings, command::CommandExecutionListener &listener) {
	core_trace_scoped(SceneGraphPanel);
	const core::String title = makeTitle(ICON_LC_WORKFLOW, _("Scene"), id);
	_hasFocus = false;

	// TODO handle dragdrop::ModelPayload with the correct parent node

	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		_hasFocus = ImGui::IsWindowHovered();
		const scenegraph::SceneGraph& sceneGraph = _sceneMgr->sceneGraph();
		const bool onlyOneModel = sceneGraph.size(scenegraph::SceneGraphNodeType::Model) <= 1;
		ui::Toolbar toolbar("toolbar");

		toolbar.button(ICON_LC_SQUARE_PLUS, _("Add a new model node"), [&sceneGraph, this, modelNodeSettings] () {
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

		toolbar.button([onlyOneModel, &listener, this] (const ImVec2 &buttonSize) {
			if (ImGui::DisabledButton(ICON_LC_PLAY, onlyOneModel, buttonSize)) {
				if (_sceneMgr->animateActive()) {
					command::executeCommands("animate 0", &listener);
				} else {
					const core::String& cmd = core::String::format("animate %f", _animationSpeedVar->floatVal());
					command::executeCommands(cmd.c_str(), &listener);
				}
			}
			ImGui::TooltipCommand("animate");
		});
		toolbar.button(ICON_LC_EYE, "showall");
		toolbar.button(ICON_LC_EYE_OFF, "hideall");
		toolbar.end();

		if (sceneGraph.nodes().size() > 10) {
			ImGui::SetNextItemWidth(ImGui::CalcTextWidth("#") * 12.0f);
			ImGui::InputText(_("Filter"), &_filterName);
			ImGui::SameLine();
			const char *filterTypes[] = {_("All"), _("Models"), _("Groups"), _("Cameras"), _("References"), _("Points")};
			static_assert(lengthof(filterTypes) == 6, "Filter types array size mismatch - see SceneGraphPanel::isFiltered");
			const float modeMaxWidth = ImGui::CalcComboWidth(filterTypes[_filterType]);
			ImGui::SetNextItemWidth(modeMaxWidth);
			if (ImGui::BeginCombo("##filtertype", filterTypes[_filterType])) {
				for (int i = 0; i < lengthof(filterTypes); i++) {
					const bool selected = i == _filterType;
					if (ImGui::Selectable(filterTypes[i], selected)) {
						_filterType = i;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		} else {
			_filterName = "";
			_filterType = 0;
		}

		static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
											ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
											ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
											ImGuiTableFlags_NoSavedSettings;
		ui::ScopedStyle style;
		style.setIndentSpacing(0.0f);
		if (ImGui::BeginTable("##nodelist", 5, tableFlags)) {
			const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
										ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

			ImGui::TableSetupScrollFreeze(0, 1);
			// TODO: UI: this space is here to align the icon a little bit - maybe there is a better way to do this
			ImGui::TableSetupColumn(" " ICON_LC_EYE "##visiblenode", colFlags);
			ImGui::TableSetupColumn(" " ICON_LC_LOCK "##lockednode", colFlags);
			ImGui::TableSetupColumn("##nodecolor", colFlags);
			ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##nodedelete", colFlags);
			ImGui::TableHeadersRow();

			int referencedNode = InvalidNodeId;
			const scenegraph::SceneGraphNode& activeNode = sceneGraph.node(sceneGraph.activeNode());
			if (activeNode.type() == scenegraph::SceneGraphNodeType::ModelReference) {
				referencedNode = activeNode.reference();
			}

			recursiveAddNodes(camera, sceneGraph, sceneGraph.node(sceneGraph.root().id()), listener, 0, referencedNode);
			ImGui::EndTable();
		}
	}
	ImGui::End();

	if (_popupDragAndDrop) {
		ImGui::OpenPopup(POPUP_TITLE_SCENEGRAPHDRAGANDDROP);
		_popupDragAndDrop = false;
	}

	registerPopups();
}

void SceneGraphPanel::registerPopups() {
	ImGuiWindowFlags popupFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginPopup(POPUP_TITLE_SCENEGRAPHDRAGANDDROP, popupFlags)) {
		const scenegraph::SceneGraph& sceneGraph = _sceneMgr->sceneGraph();
		const scenegraph::SceneGraphNode *sourceNode = _sceneMgr->sceneGraphNode(_dragDropSourceNodeId);
		const scenegraph::SceneGraphNode *targetNode = _sceneMgr->sceneGraphNode(_dragDropTargetNodeId);

		const bool canChangeParent = sceneGraph.canChangeParent(sceneGraph.node(_dragDropSourceNodeId), _dragDropTargetNodeId);
		if (sourceNode && targetNode) {
			if (sourceNode->isModelNode() && targetNode->isModelNode()) {
				if (ImGui::IconButton(ICON_LC_LINK, _("Merge onto"))) {
					_sceneMgr->mergeNodes(_dragDropTargetNodeId, _dragDropSourceNodeId);
					ImGui::CloseCurrentPopup();
				}
				ImGui::TooltipText(_("Merge %s onto %s"), sourceNode->name().c_str(), targetNode->name().c_str());
			}
		}
		if (canChangeParent) {
			scenegraph::NodeMoveFlag flags = scenegraph::NodeMoveFlag::None;
			if (ImGui::IconButton(ICON_LC_LIST_INDENT_INCREASE, _("Move below"))) {
				flags = scenegraph::NodeMoveFlag::UpdateTransform;
			}
			if (ImGui::IconButton(ICON_LC_LIST_INDENT_INCREASE, _("Move below but keep position"))) {
				flags = scenegraph::NodeMoveFlag::KeepWorldTransform;
			}
			if (flags != scenegraph::NodeMoveFlag::None) {
				if (!_sceneMgr->nodeMove(_dragDropSourceNodeId, _dragDropTargetNodeId , flags)) {
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
