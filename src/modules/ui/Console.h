/**
 * @file
 */

#pragma once

#include "util/Console.h"
#include "IMGUIEx.h"
#include "Notify.h"

namespace ui {

class IMGUIApp;

/**
 * @ingroup UI
 */
class Console : public util::Console {
private:
	using Super = util::Console;
	ImGui::ImGuiToasts _notifications;
	void addLogLine(int category, int priority, const char *message) override;

	void drawString(int x, int y, const int color[4], int, const char* str, int len) override;
	int lineHeight() override;
	glm::ivec2 stringSize(const char* s, int length) override;
	void afterRender(const math::Rect<int> &rect) override;
	void beforeRender(const math::Rect<int> &rect) override;

public:
	Console();

	void renderNotifications();
};

}
