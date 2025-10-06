/**
 * @file
 */

#include "NetworkPanel.h"
#include "IconsLucide.h"
#include "core/ConfigVar.h"
#include "imgui.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/NetworkAdapters.h"
#include "voxedit-util/network/ServerNetwork.h"

namespace voxedit {

void NetworkPanel::init() {
}

void NetworkPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NetworkPanel);
	const core::String title = makeTitle(ICON_LC_BOOK_OPEN, _("Network"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (ImGui::BeginTabBar("##networktabbar")) {
			if (ImGui::BeginTabItem(_("Client"))) {
				if (!_sceneMgr->client().isConnected()) {
					ImGui::InputVarString(_("User name"), cfg::AppUserName);
					ImGui::InputVarString(_("Host name"), cfg::VoxEditNetHostname);
					ImGui::InputVarInt(_("Port"), cfg::VoxEditNetPort);
					if (ImGui::Button(_("Connect to server"))) {
						const int port = core::Var::getSafe(cfg::VoxEditNetPort)->intVal();
						const core::String hostname = core::Var::getSafe(cfg::VoxEditNetHostname)->strVal();
						_sceneMgr->connectToServer(hostname, port);
					}
				} else {
					ImGui::TextUnformatted(_("Connected to server"));
					if (ImGui::Button(_("Disconnect"))) {
						_sceneMgr->disconnectFromServer();
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(_("Server"))) {
				ImGui::InputVarString(_("User name"), cfg::AppUserName);
				ImGui::InputVarInt(_("Max connections"), cfg::VoxEditNetServerMaxConnections);
				const core::VarPtr &portVar = core::Var::getSafe(cfg::VoxEditNetPort);
				if (_sceneMgr->server().isRunning()) {
					ImGui::Text(_("Server is running on port %i"), portVar->intVal());
					if (ImGui::Button(_("Stop server"))) {
						_sceneMgr->stopLocalServer();
					}
					for (const network::RemoteClient &client : _sceneMgr->server().clients()) {
						ImGui::BulletText("%s", client.name.c_str());
						ImGui::Text(_("Traffic: %i bytes sent, %i bytes received"), (int)client.bytesOut, (int)client.bytesIn);
					}
				} else {
					static const core::DynamicArray<core::String> &adapters = network::getNetworkAdapters();
					const core::VarPtr &ifaceVar = core::Var::getSafe(cfg::VoxEditNetServerInterface);
					const core::String &iface = ifaceVar->strVal();
					if (ImGui::BeginCombo(_("Interface"), iface.c_str())) {
						for (const core::String &ip : adapters) {
							const bool selected = iface == ip;
							if (ImGui::Selectable(ip.c_str(), selected)) {
								ifaceVar->setVal(ip);
							}
							if (selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::InputVarInt(_("Port"), portVar);
					if (ImGui::Button(_("Start Server"))) {
						const int port = portVar->intVal();
						_sceneMgr->startLocalServer(port, iface);
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Start a server to allow remote connections");
					}
				}

				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

} // namespace voxedit
