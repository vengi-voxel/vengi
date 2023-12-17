/**
 * @file
 */

#include "Console.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"
#include "imgui.h"
#include "util/Console.h"
#include <SDL_log.h>

namespace ui {


namespace _priv {

static int ConsoleInputTextCallback(ImGuiInputTextCallbackData *data) {
	Console *console = (Console *)data->UserData;
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


Console::Console() :
		Super() {
}

void Console::addLogLine(int category, int priority, const char *message) {
	Super::addLogLine(category, priority, message);
	if (priority < SDL_LOG_PRIORITY_INFO) {
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

void Console::drawString(util::ConsoleColor color, const char* str, int len) {
	// TODO: fix issue #359
	ScopedStyle style;
	switch (color) {
	case util::ConsoleColor::WHITE:
		style.setColor(ImGuiCol_Text, ImColor(255, 255, 255, 255));
		break;
	case util::ConsoleColor::RED:
		style.setColor(ImGuiCol_Text, ImColor(255, 0, 0, 255));
		break;
	case util::ConsoleColor::GREEN:
		style.setColor(ImGuiCol_Text, ImColor(0, 255, 0, 255));
		break;
	case util::ConsoleColor::BLUE:
		style.setColor(ImGuiCol_Text, ImColor(0, 0, 255, 255));
		break;
	case util::ConsoleColor::YELLOW:
		style.setColor(ImGuiCol_Text, ImColor(255, 255, 0, 255));
		break;
	case util::ConsoleColor::GRAY:
		style.setColor(ImGuiCol_Text, ImColor(128, 128, 128, 255));
		break;
	case util::ConsoleColor::BLACK:
		style.setColor(ImGuiCol_Text, ImColor(0, 0, 0, 255));
		break;
	default:
		break;
	}
	ImGui::TextUnformatted(str, str + len);
}

bool Console::render(command::CommandExecutionListener &listener) {
	if (ImGui::Begin(UI_CONSOLE_WINDOW_TITLE, nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
		for (int n = 0; n < (int)_messages.size(); ++n) {
			const core::String &msg = _messages[n];
			drawStringColored(msg, msg.size());
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
			executeCommandLine();
			ImGui::SetKeyboardFocusHere(-1);
		}
		ImGui::SameLine();
		ImGui::CommandButton("Clear", "clear", listener);
		ImGui::End();
	}
	return true;
}

void Console::renderNotifications() {
	ImGui::RenderNotifications(_notifications);
}

}
