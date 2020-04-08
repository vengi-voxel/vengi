/**
 * @file
 */

#pragma once

#include "util/Console.h"

namespace ui {
namespace nuklear {

class NuklearApp;

/**
 * @ingroup UI
 */
class Console : public util::Console {
private:
	using Super = util::Console;
	NuklearApp* _app = nullptr;

	void drawString(int x, int y, const glm::ivec4& color, int colorIndex, const char* str, int len) override;
	int lineHeight() override;
	glm::ivec2 stringSize(const char* s, int length) override;
	void afterRender(const math::Rect<int> &rect) override;
	void beforeRender(const math::Rect<int> &rect) override;

public:
	Console(NuklearApp* app);
};

}
}
