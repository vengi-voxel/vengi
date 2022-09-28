/**
 * @file
 */

#include "LayerPanel.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "core/collection/DynamicArray.h"
#include "core/Color.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUIEx.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/gtc/type_ptr.hpp>

#define LAYERPOPUP "##layerpopup"
#define POPUP_TITLE_LAYER_SETTINGS "Layer settings##popuptitle"

namespace voxedit {

void LayerPanel::addLayerItem(const voxelformat::SceneGraph& sceneGraph, voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	ImGui::TableNextColumn();

	const int nodeId = node.id();

	const core::String &visibleId = core::string::format("##visible-layer-%i", nodeId);
	bool visible = node.visible();
	if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
		sceneMgr().nodeSetVisible(nodeId, visible);
	}
	ImGui::TableNextColumn();

	const core::String &lockedId = core::string::format("##locked-layer-%i", nodeId);
	bool locked = node.locked();
	if (ImGui::Checkbox(lockedId.c_str(), &locked)) {
		sceneMgr().nodeSetLocked(nodeId, locked);
	}
	ImGui::TableNextColumn();

	core::RGBA color = node.color();
	glm::vec4 colvec = core::Color::fromRGBA(color);
	const core::String &colorId = core::string::format("Color##layer-%i", nodeId);
	if (ImGui::ColorEdit4(colorId.c_str(), glm::value_ptr(colvec), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
		node.setColor(core::Color::getRGBA(colvec));
	}
	ImGui::TableNextColumn();

	const core::String &nameId = core::string::format("##name-layer-%i", nodeId);
	ImGui::PushID(nameId.c_str());
	if (ImGui::Selectable(node.name().c_str(), nodeId == sceneGraph.activeNode())) {
		sceneMgr().nodeActivate(nodeId);
	}
	ImGui::PopID();

	const core::String &contextMenuId = core::string::format("Edit##context-layer-%i", nodeId);
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		sceneMgr().nodeActivate(nodeId);
		const int validLayers = (int)sceneGraph.size();
		ImGui::CommandMenuItem(ICON_FA_TRASH_ALT " Delete" LAYERPOPUP, "layerdelete", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_EYE_SLASH " Hide others" LAYERPOPUP, "layerhideothers", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_COPY " Duplicate" LAYERPOPUP, "layerduplicate", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_EYE " Show all" LAYERPOPUP, "layershowall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_EYE_SLASH " Hide all" LAYERPOPUP, "layerhideall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge" LAYERPOPUP, "layermerge", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge all" LAYERPOPUP, "layermergeall", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge visible" LAYERPOPUP, "layermergevisible", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_OBJECT_GROUP " Merge locked" LAYERPOPUP, "layermergelocked", validLayers > 1, &listener);
		ImGui::CommandMenuItem(ICON_FA_LOCK " Lock all" LAYERPOPUP, "layerlockall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_UNLOCK " Unlock all" LAYERPOPUP, "layerunlockall", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_COMPRESS_ARROWS_ALT " Center origin" LAYERPOPUP, "center_origin", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_COMPRESS_ARROWS_ALT " Center reference" LAYERPOPUP, "center_referenceposition", true, &listener);
		ImGui::CommandMenuItem(ICON_FA_SAVE " Save" LAYERPOPUP, "layerssave", true, &listener);
		core::String layerName = node.name();
		if (ImGui::InputText("Name" LAYERPOPUP, &layerName)) {
			sceneMgr().nodeRename(node.id(), layerName);
		}
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();

	const core::String &deleteId = core::string::format(ICON_FA_TRASH_ALT"##delete-layer-%i", nodeId);
	if (ImGui::Button(deleteId.c_str())) {
		sceneMgr().nodeRemove(nodeId, false);
	}
	ImGui::TooltipText("Delete this model");
}

void LayerPanel::update(const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener) {
	if (!_animationSpeedVar) {
		_animationSpeedVar = core::Var::getSafe(cfg::VoxEditAnimationSpeed);
	}
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	_hasFocus = false;
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		_hasFocus = ImGui::IsWindowHovered();
		const voxelformat::SceneGraph& sceneGraph = sceneMgr.sceneGraph();
		core_trace_scoped(LayerPanel);
		if (ImGui::Button(ICON_FA_PLUS_SQUARE"##newlayer")) {
			const int nodeId = sceneGraph.activeNode();
			voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
			const voxel::RawVolume* v = node.volume();
			const voxel::Region& region = v->region();
			layerSettings->position = region.getLowerCorner();
			layerSettings->size = region.getDimensionsInVoxels();
			if (layerSettings->name.empty()) {
				layerSettings->name = node.name();
			}
			ImGui::OpenPopup(POPUP_TITLE_LAYER_SETTINGS);
		}
		ImGui::TooltipText("Add a new layer");
		if (ImGui::BeginPopupModal(POPUP_TITLE_LAYER_SETTINGS, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::InputText("Name", &layerSettings->name);
			ImGui::InputVec3("Position", layerSettings->position);
			ImGui::InputVec3("Size", layerSettings->size);
			if (ImGui::Button(ICON_FA_CHECK " OK##layersettings")) {
				ImGui::CloseCurrentPopup();
				voxelformat::SceneGraphNode node;
				voxel::RawVolume* v = new voxel::RawVolume(layerSettings->region());
				node.setVolume(v, true);
				node.setName(layerSettings->name.c_str());
				sceneMgr.addNodeToSceneGraph(node);
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES " Cancel##layersettings")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}

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
		ImGui::CommandButton(ICON_FA_EYE "##layerpanel", "layershowall", nullptr, 0, &listener);
		ImGui::SameLine();
		ImGui::CommandButton(ICON_FA_EYE_SLASH "##layerpanel", "layerhideall", nullptr, 0, &listener);
		if (!onlyOneModel) {
			ImGui::InputVarFloat("Animation speed", _animationSpeedVar);
		}
		static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
										   ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
										   ImGuiTableFlags_NoSavedSettings;
		if (ImGui::BeginTable("##layerlist", 5, tableFlags)) {
			const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
									  ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

			ImGui::TableSetupColumn(ICON_FA_EYE "##visiblelayer", colFlags);
			ImGui::TableSetupColumn(ICON_FA_LOCK "##lockedlayer", colFlags);
			ImGui::TableSetupColumn("##layercolor", colFlags);
			ImGui::TableSetupColumn("Name##layer", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##deletelayer", colFlags);
			ImGui::TableHeadersRow();
			for (voxelformat::SceneGraphNode &node : sceneGraph) {
				addLayerItem(sceneGraph, node, listener);
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

bool LayerPanel::hasFocus() const {
	return _hasFocus;
}

}
