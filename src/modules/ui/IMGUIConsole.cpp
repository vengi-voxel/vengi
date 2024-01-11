/**
 * @file
 */

#include "IMGUIConsole.h"
#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "IconsLucide.h"
#include "ScopedStyle.h"
#include "command/CommandHandler.h"
#include "core/Log.h"
#include "imgui.h"
#include "util/Console.h"

namespace ui {


namespace _priv {

static int ConsoleInputTextCallback(ImGuiInputTextCallbackData *data) {
	IMGUIConsole *console = (IMGUIConsole *)data->UserData;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
		// make a copy here
		console->autoComplete();
		const core::String commandLine = console->commandLine();
		data->DeleteChars(0, data->BufTextLen);
		data->InsertChars(0, commandLine.c_str());
	} else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
		if (data->EventKey == ImGuiKey_UpArrow) {
			console->cursorUp();
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, console->commandLine().c_str());
		} else if (data->EventKey == ImGuiKey_DownArrow) {
			console->cursorDown();
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, console->commandLine().c_str());
		}
	}
	return 0;
}

}


void IMGUIConsole::addLogLine(int category, Log::Level priority, const char *message) {
	Super::addLogLine(category, priority, message);
	if (priority <= Log::Level::Info) {
		return;
	}

	int toastType;
	switch (priority) {
	case Log::Level::Trace:
	case Log::Level::Debug:
		toastType = ImGuiToastType_Debug;
		break;
	case Log::Level::Warn:
		toastType = ImGuiToastType_Warning;
		break;
	case Log::Level::Error:
		toastType = ImGuiToastType_Error;
		break;
	case Log::Level::Info:
		toastType = ImGuiToastType_Info;
		break;
	default:
		return;
	}
	const core::String& rawMsg = removeAnsiColors(message);
	_notifications.emplace_back(toastType, rawMsg);
}

void IMGUIConsole::drawString(const Message& msg) {
	ScopedStyle style;
	switch (msg.priority) {
	case Log::Level::Warn:
		style.setColor(ImGuiCol_Text, ImColor(255, 127, 0, 255));
		break;
	case Log::Level::Error:
		style.setColor(ImGuiCol_Text, ImColor(255, 0, 0, 255));
		break;
	default:
		break;
	}
	ImGui::TextUnformatted(msg.message.c_str());
}

bool IMGUIConsole::render(command::CommandExecutionListener &listener) {
	if (ImGui::Begin(UI_CONSOLE_WINDOW_TITLE, nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu(ICON_LC_FILE " File")) {
				ImGui::CommandMenuItem(ICON_LC_LIST_X " Clear", "con_clear", true, &listener);
				ImGui::Separator();
				if (ImGui::Button(ICON_LC_COPY " Copy to clipboard")) {
					ImGui::LogToClipboard();
					for (const Message &msg : _messages) {
						ImGui::TextUnformatted(msg.message.c_str());
					}
					ImGui::LogFinish();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(ICON_LC_MENU " Options")) {
				bool debug = (Log::Level)core::Var::getSafe(cfg::CoreLogLevel)->intVal() <= Log::Level::Debug;
				if (ImGui::Checkbox("Debug", &debug)) {
					core::Var::getSafe(cfg::CoreLogLevel)->setVal(debug ? (int)Log::Level::Debug : (int)Log::Level::Info);
				}
				ImGui::TooltipText("Enable debug logging for the console");
				ImGui::Checkbox(ICON_LC_LOCK " Auto scrolling", &_autoScrollEnabled);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
		for (int n = 0; n < (int)_messages.size(); ++n) {
			const Message &msg = _messages[n];
			drawString(msg);
		}

		if (_autoScrollEnabled && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			ImGui::SetScrollHereY(1.0f);
		}

 		ImGui::EndChild();
		ImGui::TextUnformatted(_consolePrompt.c_str());
		ImGui::SameLine();
		if (ImGui::InputText("##console-input-text", &_commandLine,
							 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
								 ImGuiInputTextFlags_CallbackHistory,
							 _priv::ConsoleInputTextCallback, this)) {
			executeCommandLine(&imguiApp()->commandListener());
			ImGui::SetKeyboardFocusHere(-1);
		}
	}
	ImGui::End();
	return true;
}

void IMGUIConsole::renderNotifications() {
	ImGui::RenderNotifications(_notifications);
}

}
