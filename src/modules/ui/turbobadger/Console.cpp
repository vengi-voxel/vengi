#include "Console.h"
#include "TurboBadger.h"
#include "FontUtil.h"

namespace ui {
namespace turbobadger {

static const tb::TBColor consoleBgColor(127, 127, 127, 150);

Console::Console() :
		Super() {
}

bool Console::init() {
	if (!Super::init()) {
		return false;
	}
	_font = getMonoSpaceFont(14);
	return true;
}

bool Console::toggle() {
	const bool active = Super::toggle();
	if (active) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
	return active;
}

void Console::drawString(int x, int y, const glm::ivec4& color, const char* str, int len) {
	_font->drawString(x, y, tb::TBColor{color.r, color.g, color.b}, str, len);
}

void Console::beforeRender(const math::Rect<int> &rect) {
	const tb::TBRect r(rect.getMinX(), rect.getMinZ(), rect.getMaxX(), rect.getMaxZ());
	tb::g_tb_skin->paintRectFill(r, consoleBgColor);
}

int Console::lineHeight() {
	const int lineHeight = _font->getFontDescription().getSize();
	return lineHeight;
}

glm::ivec2 Console::stringSize(const char* s, int length) {
	return glm::ivec2(_font->getStringWidth(s, length), lineHeight());
}

}
}
