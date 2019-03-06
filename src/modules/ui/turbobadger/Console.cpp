#include "Console.h"
#include "TurboBadger.h"
#include "FontUtil.h"

namespace ui {
namespace turbobadger {

Console::Console() :
		Super() {
}

void Console::construct() {
	Super::construct();
	_consoleAlpha = core::Var::get("ui_consolealpha", "0.9", "Console background alpha value between 0.0 and 1.0");
	_consoleBackground = core::Var::get("ui_consolebackground", "0.1", "Console background gray color value between 0.0 and 1.0");
	_consoleFontSize = core::Var::get("ui_consolefontsize", "14", "Console font size");
}

bool Console::init() {
	if (!Super::init()) {
		return false;
	}
	_font = getMonoSpaceFont(_consoleFontSize->intVal());
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
	if (_consoleFontSize->isDirty()) {
		_font = getMonoSpaceFont(_consoleFontSize->intVal());
		_consoleFontSize->markClean();
	}
	const tb::TBRect r(rect.getMinX(), rect.getMinZ(), rect.getMaxX(), rect.getMaxZ());
	const int c = glm::clamp((int)(_consoleBackground->floatVal() * core::Color::magnitudef), 0, 255);
	const int a = glm::clamp((int)(_consoleAlpha->floatVal() * core::Color::magnitudef), 0, 255);
	const tb::TBColor consoleBgColor(c, c, c, a);
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
