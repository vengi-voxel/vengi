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
#include "ui/Style.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/ServerNetwork.h"

namespace voxedit {

void NetworkPanel::init() {
}

void NetworkPanel::renderChat() {
	ImGui::Separator();
	ImGui::Text("%s %s", ICON_LC_MESSAGE_CIRCLE, _("Chat"));

	const core::DynamicArray<ChatEntry> &chatLog = _sceneMgr->client().chatLog();
	const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("##chatlog", ImVec2(0, -footerHeight), ImGuiChildFlags_Borders)) {
		for (size_t i = 0; i < chatLog.size(); ++i) {
			const ChatEntry &entry = chatLog[i];
			if (entry.system) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(style::color(style::ColorChatSystem)));
				ImGui::Text("* %s", entry.message.c_str());
				ImGui::PopStyleColor();
			} else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(style::color(style::ColorChatSender)));
				ImGui::Text("%s:", entry.sender.c_str());
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::TextWrapped("%s", entry.message.c_str());
			}
		}
		if (_scrollToBottom) {
			ImGui::SetScrollHereY(1.0f);
			_scrollToBottom = false;
		}
	}
	ImGui::EndChild();

	// Chat input
	bool sendChat = false;
	if (_refocusChatInput) {
		ImGui::SetKeyboardFocusHere();
		_refocusChatInput = false;
		_justRefocused = true;
	}
	const float sendButtonWidth = ImGui::CalcTextSize(_("Send")).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	ImGui::PushItemWidth(-sendButtonWidth);
	struct ChatInputCallbackData {
		bool *justRefocused;
		int *pendingCursorPos;
	};
	ChatInputCallbackData callbackData{&_justRefocused, &_pendingCursorPos};
	auto chatInputCallback = [](ImGuiInputTextCallbackData *data) -> int {
		ChatInputCallbackData *ctx = (ChatInputCallbackData *)data->UserData;
		if (*ctx->justRefocused) {
			// Position cursor at the requested location without selecting all text
			const int pos = (*ctx->pendingCursorPos >= 0) ? *ctx->pendingCursorPos : data->BufTextLen;
			data->CursorPos = pos;
			data->SelectionStart = data->SelectionEnd = pos;
			*ctx->justRefocused = false;
			*ctx->pendingCursorPos = -1;
		}
		return 0;
	};
	if (ImGui::InputText("##chatinput", &_chatInput,
						 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
							 ImGuiInputTextFlags_CallbackAlways,
						 chatInputCallback, &callbackData)) {
		sendChat = true;
	}
	ImGui::PopItemWidth();

	// Handle @mention autocomplete popup
	handleMentionAutocomplete();

	ImGui::SameLine();
	if (ImGui::Button(_("Send"))) {
		sendChat = true;
	}

	if (sendChat && !_chatInput.empty()) {
		_sceneMgr->client().sendChat(_chatInput);
		_chatInput.clear();
		_scrollToBottom = true;
		_refocusChatInput = true;
	}
}

void NetworkPanel::handleMentionAutocomplete() {
	// Find the last @ in the input
	const size_t atPos = _chatInput.rfind("@");
	if (atPos == core::String::npos) {
		_mentionPopupOpen = false;
		return;
	}

	// Extract the partial name after @
	const core::String partial = _chatInput.substr(atPos + 1);

	Client &networkClient = _sceneMgr->client();
	core::DynamicArray<core::String> matches;
	const core::DynamicArray<ClientInfo> &connectedClients = networkClient.connectedClients();
	for (size_t i = 0; i < connectedClients.size(); ++i) {
		const ClientInfo &info = connectedClients[i];
		if (info.name.empty()) {
			continue;
		}
		const core::String &dispName = networkClient.disambiguatedName(info);
		if (partial.empty() || core::string::icontains(dispName, partial)) {
			matches.push_back(dispName);
		}
	}

	if (matches.empty()) {
		_mentionPopupOpen = false;
		return;
	}

	// Open popup when the chat input is active; keep it open until user selects or clears @
	if (ImGui::IsItemActive()) {
		_mentionPopupOpen = true;
	}

	if (!_mentionPopupOpen) {
		return;
	}

	// Show autocomplete popup below the chat input
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y), ImGuiCond_Always);
	const float dpiScale = ImGui::GetStyle().FontScaleDpi;
	ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f * dpiScale, 0), ImVec2(300.0f * dpiScale, 150.0f * dpiScale));
	constexpr ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
											ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
											ImGuiWindowFlags_AlwaysAutoResize;
	if (ImGui::Begin("##mentionpopup", nullptr, popupFlags)) {
		for (size_t i = 0; i < matches.size(); ++i) {
			if (ImGui::Selectable(matches[i].c_str())) {
				// Replace the @partial with @username and position cursor right after the inserted text
				_chatInput = _chatInput.substr(0, atPos) + "@" + matches[i] + " ";
				_pendingCursorPos = (int)(atPos + 1 + matches[i].size() + 1);
				_mentionPopupOpen = false;
				_refocusChatInput = true;
			}
		}
	}
	ImGui::End();
}

void NetworkPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NetworkPanel);
	const core::String title = makeTitle(ICON_LC_NETWORK, _("Network"), id);
	Client &networkClient = _sceneMgr->client();

	// Register chat callback for notification sounds (once)
	if (!_chatCallbackRegistered) {
		const core::String ownName = core::getVar(cfg::AppUserName)->strVal();
		networkClient.setChatCallback([this, ownName](const ChatEntry &entry) {
			_scrollToBottom = true;
			// Check for @mention of our own name
			if (!entry.system && !ownName.empty()) {
				core::String mention = "@" + ownName;
				if (core::string::icontains(entry.message, mention)) {
					_sceneMgr->soundManager().playSound(_sceneMgr->chatSound());
				}
			}
		});
		_chatCallbackRegistered = true;
	}

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
					if (ImGui::InputText(_("Command"), &_command,
										 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll)) {
						networkClient.executeCommand(_command);
						ImGui::SetKeyboardFocusHere(-1);
					}
					if (ImGui::Button(_("Disconnect"))) {
						command::Command::execute("net_client_disconnect");
					}

					renderChat();
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
						constexpr ImGuiTableFlags tableFlags =
							ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
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

					if (networkClient.isConnected()) {
						renderChat();
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
