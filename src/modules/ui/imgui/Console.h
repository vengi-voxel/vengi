/**
 * @file
 */

#pragma once

#include "util/Console.h"
#include "IMGUI.h"

namespace ui {
namespace imgui {

class IMGUIApp;

/**
 * @ingroup UI
 */
class Console : public util::Console {
private:
	using Super = util::Console;

	void drawString(int x, int y, const glm::ivec4& color, int, const char* str, int len) override;
	int lineHeight() override;
	glm::ivec2 stringSize(const char* s, int length) override;
	void afterRender(const math::Rect<int> &rect) override;
	void beforeRender(const math::Rect<int> &rect) override;

public:
	Console();
};

}
}
