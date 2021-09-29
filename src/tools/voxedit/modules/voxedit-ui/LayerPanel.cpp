/**
 * @file
 */

#include "LayerPanel.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

#define LAYERPOPUP "##layerpopup"
#define POPUP_TITLE_LAYER_SETTINGS "Layer settings##popuptitle"

namespace voxedit {

static void executeItem(const char *title, const char *command, bool enable, command::CommandExecutionListener &listener) {
	if (ImGui::CommandMenuItem(title, command, enable)) {
		static core::DynamicArray<core::String> args(0);
		listener(command, args);
	}
}

void LayerPanel::addLayerItem(int layerId, const voxedit::Layer &layer, command::CommandExecutionListener &listener) {
	voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
	ImGui::TableNextColumn();

	const core::String &visibleId = core::string::format("##visible-layer-%i", layerId);
	bool visible = layer.visible;
	if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
		layerMgr.hideLayer(layerId, !visible);
	}
	ImGui::TableNextColumn();

	const core::String &lockedId = core::string::format("##locked-layer-%i", layerId);
	bool locked = layer.locked;
	if (ImGui::Checkbox(lockedId.c_str(), &locked)) {
		layerMgr.lockLayer(layerId, locked);
	}
	ImGui::TableNextColumn();

	const core::String &nameId = core::string::format("##name-layer-%i", layerId);
	ImGui::PushID(nameId.c_str());
	if (ImGui::Selectable(layer.name.c_str(), layerId == layerMgr.activeLayer())) {
		layerMgr.setActiveLayer(layerId);
	}
	ImGui::PopID();

	const core::String &contextMenuId = core::string::format("Edit##context-layer-%i", layerId);
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		const int validLayers = layerMgr.validLayers();
		executeItem(ICON_FA_TRASH_ALT " Delete" LAYERPOPUP, "layerdelete", validLayers > 1, listener);
		executeItem(ICON_FA_EYE_SLASH " Hide others" LAYERPOPUP, "layerhideothers", validLayers > 1, listener);
		executeItem(ICON_FA_COPY " Duplicate" LAYERPOPUP, "layerduplicate", true, listener);
		executeItem(ICON_FA_EYE " Show all" LAYERPOPUP, "layershowall", true, listener);
		executeItem(ICON_FA_EYE_SLASH " Hide all" LAYERPOPUP, "layerhideall", true, listener);
		executeItem(ICON_FA_CARET_SQUARE_UP " Move up" LAYERPOPUP, "layermoveup", validLayers > 1, listener);
		executeItem(ICON_FA_CARET_SQUARE_DOWN " Move down" LAYERPOPUP, "layermovedown", validLayers > 1, listener);
		executeItem(ICON_FA_OBJECT_GROUP " Merge" LAYERPOPUP, "layermerge", validLayers > 1, listener);
		executeItem(ICON_FA_LOCK " Lock all" LAYERPOPUP, "layerlockall", true, listener);
		executeItem(ICON_FA_UNLOCK " Unlock all" LAYERPOPUP, "layerunlockall", true, listener);
		executeItem(ICON_FA_COMPRESS_ARROWS_ALT " Center origin" LAYERPOPUP, "center_origin", true, listener);
		executeItem(ICON_FA_COMPRESS_ARROWS_ALT " Center reference" LAYERPOPUP, "center_referenceposition", true, listener);
		executeItem(ICON_FA_SAVE " Save" LAYERPOPUP, "layerssave", true, listener);
		core::String layerName = layer.name;
		if (ImGui::InputText("Name" LAYERPOPUP, &layerName)) {
			layerMgr.rename(layerId, layerName);
		}
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();

	const core::String &deleteId = core::string::format(ICON_FA_TRASH_ALT"##delete-layer-%i", layerId);
	if (ImGui::Button(deleteId.c_str())) {
		layerMgr.deleteLayer(layerId);
	}
}

void LayerPanel::update(const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener) {
	if (!_animationSpeedVar) {
		_animationSpeedVar = core::Var::getSafe(cfg::VoxEditAnimationSpeed);
	}
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		ImGui::BeginChild("##layertable", ImVec2(0.0f, 400.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
		static const uint32_t TableFlags =
			ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable |
			ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg;
		if (ImGui::BeginTable("##layerlist", 4, TableFlags)) {
			ImGui::TableSetupColumn(ICON_FA_EYE"##visiblelayer", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(ICON_FA_LOCK"##lockedlayer", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Name##layer", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##deletelayer", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
			const voxedit::Layers& layers = layerMgr.layers();
			const int layerCnt = layers.size();
			for (int l = 0; l < layerCnt; ++l) {
				if (!layers[l].valid) {
					continue;
				}
				addLayerItem(l, layers[l], listener);
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();
		if (ImGui::Button(ICON_FA_PLUS_SQUARE"##newlayer")) {
			const int layerId = layerMgr.activeLayer();
			const voxel::RawVolume* v = sceneMgr.volume(layerId);
			const voxel::Region& region = v->region();
			layerSettings->position = region.getLowerCorner();
			layerSettings->size = region.getDimensionsInVoxels();
			if (layerSettings->name.empty()) {
				layerSettings->name = layerMgr.layer(layerId).name;
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
				voxel::RawVolume* v = new voxel::RawVolume(layerSettings->region());
				const int layerId = layerMgr.addLayer(layerSettings->name.c_str(), true, v, v->region().getCenter());
				layerMgr.setActiveLayer(layerId);
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES " Cancel##layersettings")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		const bool multipleLayers = layerMgr.validLayers() <= 1;
		if (ImGui::DisabledButton(ICON_FA_PLAY"##animatelayers", multipleLayers)) {
			if (sceneMgr.animateActive()) {
				command::executeCommands("animate 0", &listener);
			} else {
				const core::String& cmd = core::string::format("animate %f", _animationSpeedVar->floatVal());
				command::executeCommands(cmd.c_str(), &listener);
			}
		}
		ImGui::SameLine();
		if (ImGui::DisabledButton(ICON_FA_CARET_SQUARE_UP"##layers", multipleLayers)) {
			command::executeCommands("layermoveup", &listener);
		}
		ImGui::TooltipText("Move the layer one level up");
		ImGui::SameLine();
		if (ImGui::DisabledButton(ICON_FA_CARET_SQUARE_DOWN "##layers", multipleLayers)) {
			command::executeCommands("layermovedown", &listener);
		}
		ImGui::TooltipText("Move the layer one level down");
		if (!multipleLayers) {
			ImGui::InputVarFloat("Animation speed", _animationSpeedVar);
		}
	}
	ImGui::End();
}

}
