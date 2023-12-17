/**
 * @file
 */

#pragma once

#include "Notify.h"
#include "command/CommandHandler.h"
#include "util/Console.h"

namespace ui {

class IMGUIApp;

#define UI_CONSOLE_WINDOW_TITLE "Console"

/**
 * @ingroup UI
 */
class Console : public util::Console {
private:
	using Super = util::Console;

	bool _autoScrollEnabled = true;
	ImGui::ImGuiToasts _notifications;
	void addLogLine(int category, int priority, const char *message) override;

	void drawString(util::ConsoleColor color, const char *str, int len) override;

public:
	Console();

	bool render(command::CommandExecutionListener &listener);
	void renderNotifications();
};

} // namespace ui
