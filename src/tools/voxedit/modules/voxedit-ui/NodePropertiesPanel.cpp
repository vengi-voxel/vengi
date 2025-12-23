/**
 * @file
 */

#include "NodePropertiesPanel.h"
#include "ScopedID.h"
#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedStyle.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool NodePropertiesPanel::init() {
	return true;
}

void NodePropertiesPanel::shutdown() {
}

bool NodePropertiesPanel::handleCameraProperty(const scenegraph::SceneGraphNodeCamera &node, const core::String &key,
											   const core::String &value) {
	if (key == scenegraph::PropCamMode) {
		int currentMode = value == scenegraph::SceneGraphNodeCamera::Modes[0] ? 0 : 1;

		if (ImGui::BeginCombo("##cammode", scenegraph::SceneGraphNodeCamera::Modes[currentMode])) {
			for (int n = 0; n < lengthof(scenegraph::SceneGraphNodeCamera::Modes); n++) {
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
		if (ImGui::InputFloat("##val", &fvalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			_sceneMgr->nodeSetProperty(node.id(), key, core::string::toString(fvalue));
		}
	} else if (scenegraph::SceneGraphNodeCamera::isIntProperty(key)) {
		int ivalue = core::string::toInt(value);
		if (ImGui::InputInt("##val", &ivalue, ImGuiInputTextFlags_EnterReturnsTrue)) {
			_sceneMgr->nodeSetProperty(node.id(), key, core::string::toString(ivalue));
		}
	} else {
		return false;
	}
	return true;
}

void NodePropertiesPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NodeInspectorPanel);
	const core::String title = makeTitle(ICON_LC_LOCATE, _("Node Properties"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		scenegraph::SceneGraphNode &node = sceneGraph.node(sceneGraph.activeNode());

		core::String deleteKey;
		static const uint32_t tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
										   ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg |
										   ImGuiTableFlags_NoSavedSettings;
		ui::ScopedStyle style;
		style.setIndentSpacing(0.0f);
		if (ImGui::BeginTable("##nodeproperties", 3, tableFlags)) {
			const uint32_t colFlags = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
									  ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;

			ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##nodepropertydelete", colFlags);
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::TextUnformatted(_("UUID"));
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			const core::String &uuidStr = node.uuid().str();
			ImGui::TextUnformatted(uuidStr.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::TextUnformatted(_("Name"));
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			if (ImGui::Selectable(node.name().c_str())) {
				command::executeCommands("toggle ve_popuprenamenode", &listener);
			}

			ImGuiListClipper clipper;
			clipper.Begin(node.properties().size());
			while (clipper.Step()) {
				auto entry = core::next(node.properties().begin(), clipper.DisplayStart);
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row, ++entry) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-1);
					ImGui::TextUnformatted(entry->key.c_str());
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-1);

					ui::ScopedID scopedId(entry->key);
					bool propertyAlreadyHandled = false;
					if (node.isCameraNode()) {
						propertyAlreadyHandled =
							handleCameraProperty(scenegraph::toCameraNode(node), entry->key, entry->value);
					}

					if (!propertyAlreadyHandled) {
						if (entry->value == "true" || entry->value == "false") {
							bool value = core::string::toBool(entry->value);
							if (ImGui::Checkbox("##val", &value)) {
								_sceneMgr->nodeSetProperty(node.id(), entry->key, core::string::toString(value));
							}
						} else {
							core::String value = entry->value;
							if (ImGui::InputText("##val", &value,
												 ImGuiInputTextFlags_EnterReturnsTrue |
													 ImGuiInputTextFlags_AutoSelectAll)) {
								_sceneMgr->nodeSetProperty(node.id(), entry->key, value);
							}
						}
					}
					ImGui::TableNextColumn();
					if (ImGui::Button(ICON_LC_X)) {
						deleteKey = entry->key;
					}
					ImGui::TooltipTextUnformatted(_("Delete this node property"));
				}
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##newpropertykey", &_propertyKey);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##newpropertyvalue", &_propertyValue);
			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_LC_PLUS "###nodepropertyadd")) {
				_sceneMgr->nodeSetProperty(node.id(), _propertyKey, _propertyValue);
				_propertyKey = _propertyValue = "";
			}
			ImGui::TooltipTextUnformatted(_("Add a new node property"));

			ImGui::EndTable();
		}

		if (!deleteKey.empty()) {
			_sceneMgr->nodeRemoveProperty(node.id(), deleteKey);
		}
	}
	ImGui::End();
}

} // namespace voxedit
