/**
 * @file
 */

#pragma once

#include "util/Console.h"

namespace tb {
class TBFontFace;
}

namespace ui {

class Console : public util::Console {
private:
	using Super = util::Console;
	tb::TBFontFace *_font = nullptr;

	void drawString(int x, int y, const glm::ivec4& color, const char* str, int len) override;
	int lineHeight() override;
	int stringWidth(const char* s, int length) override;
	void beforeRender(const math::Rect<int> &rect) override;

public:
	Console();
	bool init() override;
	bool toggle() override;
};

}
