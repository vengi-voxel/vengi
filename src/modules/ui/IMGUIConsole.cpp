/**
 * @file
 */

#include "IMGUIConsole.h"
#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"
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
		ImGui::SameLine();
		// don't offer trace here - it would flood the logs for the user
		static const char *LogLevelNames[] = {Log::toLogLevel(Log::Level::Debug), Log::toLogLevel(Log::Level::Info),
											  Log::toLogLevel(Log::Level::Warn), Log::toLogLevel(Log::Level::Error)};
		int currentLogLevel = core::Var::getSafe(cfg::CoreLogLevel)->intVal();
		if (ImGui::Combo("Log Level", &currentLogLevel, LogLevelNames, IM_ARRAYSIZE(LogLevelNames))) {
			const char *logLevelNew = LogLevelNames[currentLogLevel];
			core::Var::getSafe(cfg::CoreLogLevel)->setVal((int)Log::toLogLevel(logLevelNew));
		}
		// TODO: save log to file
		ImGui::End();
	}
	return true;
}

void IMGUIConsole::renderNotifications() {
	ImGui::RenderNotifications(_notifications);
}

}
