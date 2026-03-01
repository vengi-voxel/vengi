/**
 * @file
 */

#include "NetworkPanel.h"
#include "IconsLucide.h"
#include "command/Command.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "network/NetworkAdapters.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/ServerNetwork.h"

namespace voxedit {

void NetworkPanel::init() {
}

void NetworkPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NetworkPanel);
	const core::String title = makeTitle(ICON_LC_NETWORK, _("Network"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (ImGui::BeginTabBar("##networktabbar")) {
			if (ImGui::BeginTabItem(_("Client"))) {
				if (!_sceneMgr->client().isConnected()) {
					ImGui::InputVarString(cfg::AppUserName);
					ImGui::InputVarString(cfg::VoxEditNetPassword);
					ImGui::InputVarString(cfg::VoxEditNetHostname);
					ImGui::InputVarInt(cfg::VoxEditNetPort);
					if (ImGui::Button(_("Connect to server"))) {
						command::Command::execute("net_client_connect");
					}
				} else {
					ImGui::TextUnformatted(_("Connected to server"));
					ImGui::InputVarString(cfg::VoxEditNetRconPassword);
					if (ImGui::Button(_("New Scene"))) {
						_sceneMgr->client().executeCommand("newscene");
					}
					if (ImGui::InputText(_("Command"), &_command,
										 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll)) {
						_sceneMgr->client().executeCommand(_command);
						ImGui::SetKeyboardFocusHere(-1);
					}
					if (ImGui::Button(_("Disconnect"))) {
						command::Command::execute("net_client_disconnect");
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(_("Server"))) {
				ImGui::InputVarString(cfg::AppUserName);
				ImGui::InputVarString(cfg::VoxEditNetPassword);
				ImGui::InputVarString(cfg::VoxEditNetRconPassword);
				ImGui::InputVarInt(cfg::VoxEditNetServerMaxConnections);
				const core::VarPtr &portVar = core::getVar(cfg::VoxEditNetPort);
				if (_sceneMgr->server().isRunning()) {
					ImGui::Text(_("Server is running on port %i"), portVar->intVal());
					if (ImGui::Button(_("Stop server"))) {
						command::Command::execute("net_server_stop");
					}
					const RemoteClients &clients = _sceneMgr->server().clients();
					if (!clients.empty()) {
						constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
						if (ImGui::BeginTable("##clients", 4, tableFlags)) {
							ImGui::TableSetupColumn(_("Name"));
							ImGui::TableSetupColumn(_("Sent"));
							ImGui::TableSetupColumn(_("Received"));
							ImGui::TableSetupColumn(_("Actions"));
							ImGui::TableHeadersRow();
							for (int i = 0; i < (int)clients.size(); ++i) {
								const RemoteClient &client = clients[i];
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::TextUnformatted(client.name.c_str());
								ImGui::TableSetColumnIndex(1);
								ImGui::Text("%s", core::string::humanSize(client.bytesOut).c_str());
								ImGui::TableSetColumnIndex(2);
								ImGui::Text("%s", core::string::humanSize(client.bytesIn).c_str());
								ImGui::TableSetColumnIndex(3);
								ImGui::PushID(i);
								if (ImGui::SmallButton(_("Kick"))) {
									command::Command::execute(core::String::format("net_server_kick %i", i));
								}
								ImGui::PopID();
							}
							ImGui::EndTable();
						}
					}
				} else {
					static const core::DynamicArray<core::String> &adapters = network::getNetworkAdapters();
					const core::VarPtr &ifaceVar = core::getVar(cfg::VoxEditNetServerInterface);
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
					ImGui::InputVarInt(portVar);
					if (ImGui::Button(_("Start Server"))) {
						command::Command::execute("net_server_start");
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
