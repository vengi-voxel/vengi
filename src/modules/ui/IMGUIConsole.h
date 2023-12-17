/**
 * @file
 */

#pragma once

#include "Notify.h"
#include "command/CommandHandler.h"
#include "util/Console.h"

namespace ui {

#define UI_CONSOLE_WINDOW_TITLE "Console"

/**
 * @ingroup UI
 */
class IMGUIConsole : public util::Console {
private:
	using Super = util::Console;

	bool _autoScrollEnabled = true;
	ImGui::ImGuiToasts _notifications;
	void addLogLine(int category, Log::Level priority, const char *message) override;
	void drawString(const Message& msg);

public:
	bool render(command::CommandExecutionListener &listener);
	void renderNotifications();
};

} // namespace ui
