/**
 * @file
 */

#pragma once

#include "util/Console.h"
#include "core/Var.h"

namespace tb {
class TBFontFace;
}

namespace ui {
namespace turbobadger {

class Console : public util::Console {
private:
	using Super = util::Console;
	tb::TBFontFace *_font = nullptr;
	core::VarPtr _consoleAlpha;
	core::VarPtr _consoleBackground;
	core::VarPtr _consoleFontSize;

	void drawString(int x, int y, const glm::ivec4& color, int, const char* str, int len) override;
	int lineHeight() override;
	glm::ivec2 stringSize(const char* s, int length) override;
	void beforeRender(const math::Rect<int> &rect) override;

public:
	Console();
	void construct() override;
	bool init() override;
	bool toggle() override;
};

}
}
