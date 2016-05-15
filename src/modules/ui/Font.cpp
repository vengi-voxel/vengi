#include "Font.h"
#include "core/Color.h"

namespace ui {

bool Font::init(const char *font, int size) {
	tb::TBFontDescription fd;
	fd.SetID(TBIDC(font));
	fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(size));
	color(core::Color::White);

	tb::TBFontManager *fontMgr = tb::g_font_manager;

	if (fontMgr->HasFontFace(fd)) {
		_font = fontMgr->GetFontFace(fd);
	} else {
		_font = fontMgr->CreateFontFace(fd);
	}

	if (_font == nullptr) {
		Log::warn("Could not create font '%s'", font);
		return false;
	}

	core_assert(_font->RenderGlyphs(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNORSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~•·åäöÅÄÖ"));

	return true;
}

bool Font::init(tb::TBFontFace *font) {
	if (font == nullptr) {
		return false;
	}
	_font = font;
	color(core::Color::White);
	return true;
}

Font& Font::draw(const char *format, ...) {
	core_assert_msg(_font != nullptr, "Font not yet initialized");
	va_list args;
	va_start(args, format);
	char buf[_maxLength];
	std::vsnprintf(buf, sizeof(buf), format, args);
	const tb::TBColor color(_color.r, _color.g, _color.b, _color.a);
	_font->DrawString(_pos.x, _pos.y, color, buf);
	_pos.y += getSize();
	return *this;
}

}
