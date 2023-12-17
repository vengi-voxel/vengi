/**
 * @file
 */

#include "IMGUIConsole.h"
#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"
#include "imgui.h"
#include "util/Console.h"
#include <SDL_log.h>

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


void IMGUIConsole::addLogLine(int category, int priority, const char *message) {
	Super::addLogLine(category, priority, message);
	if (priority <= SDL_LOG_PRIORITY_INFO) {
		return;
	}

	int toastType;
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
	case SDL_LOG_PRIORITY_DEBUG:
		toastType = ImGuiToastType_Debug;
		break;
	case SDL_LOG_PRIORITY_WARN:
		toastType = ImGuiToastType_Warning;
		break;
	case SDL_LOG_PRIORITY_ERROR:
	case SDL_LOG_PRIORITY_CRITICAL:
		toastType = ImGuiToastType_Error;
		break;
	case SDL_LOG_PRIORITY_INFO:
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
	case SDL_LOG_PRIORITY_WARN:
		style.setColor(ImGuiCol_Text, ImColor(255, 127, 0, 255));
		break;
	case SDL_LOG_PRIORITY_ERROR:
	case SDL_LOG_PRIORITY_CRITICAL:
		style.setColor(ImGuiCol_Text, ImColor(255, 0, 0, 255));
		break;
	case SDL_LOG_PRIORITY_VERBOSE:
	case SDL_LOG_PRIORITY_DEBUG:
	case SDL_LOG_PRIORITY_INFO:
	default:
		break;
	}
	ImGui::TextUnformatted(msg.message.c_str());
}

bool IMGUIConsole::render(command::CommandExecutionListener &listener) {
	if (ImGui::Begin(UI_CONSOLE_WINDOW_TITLE, nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
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
		ImGui::SameLine();
		ImGui::CommandButton("Clear", "clear", listener);
		ImGui::End();
	}
	return true;
}

void IMGUIConsole::renderNotifications() {
	ImGui::RenderNotifications(_notifications);
}

}
