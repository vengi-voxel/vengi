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
		} else if (data->EventKey == ImGuiKey_DownArrow) {
			console->cursorDown();
		}
		// take a copy - cursorUp/cursorDown modify _commandLine which may
		// free the buffer that data->Buf still points to. DeleteChars and
		// InsertChars will trigger the resize callback that keeps data->Buf
		// and _commandLine in sync.
		const core::String commandLine = console->commandLine();
		data->DeleteChars(0, data->BufTextLen);
		data->InsertChars(0, commandLine.c_str());
	}
	return 0;
}

} // namespace _priv

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
	const core::String &rawMsg = removeAnsiColors(message);
	_notifications.emplace_back(toastType, rawMsg);
}

void IMGUIConsole::drawString(const Message &msg) {
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
			if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
				ImGui::CommandIconMenuItem(ICON_LC_LIST_X, _("Clear"), "con_clear", true, &listener);
				ImGui::Separator();
				if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_COPY, _("Copy"))) {
					ImGui::LogToClipboard();
					for (const Message &msg : _messages) {
						ImGui::TextUnformatted(msg.message.c_str());
					}
					ImGui::LogFinish();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
				bool debug = (Log::Level)core::getVar(cfg::CoreLogLevel)->intVal() <= Log::Level::Debug;
				if (ImGui::Checkbox(_("Debug"), &debug)) {
					core::getVar(cfg::CoreLogLevel)->setVal(debug ? (int)Log::Level::Debug : (int)Log::Level::Info);
				}
				ImGui::TooltipTextUnformatted(_("Enable debug logging for the console"));
				ImGui::IconCheckbox(ICON_LC_LOCK, _("Auto scrolling"), &_autoScrollEnabled);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar);

		ImGuiListClipper clipper;
		clipper.Begin(_messages.size(), ImGui::GetTextLineHeightWithSpacing());
		while (clipper.Step()) {
			for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
				const Message &msg = _messages[line_no];
				drawString(msg);
			}
		}
		if (_autoScrollEnabled && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
		if (ImGui::InputText(_("Command"), &_commandLine,
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
								 ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory,
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

} // namespace ui
