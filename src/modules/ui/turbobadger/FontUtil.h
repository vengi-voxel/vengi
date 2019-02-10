/**
 * @file
 */

#pragma once

#include "TurboBadger.h"
#include "core/Common.h"

extern void register_tbbf_font_renderer();
extern void register_stb_font_renderer();

namespace ui {
namespace turbobadger {

static const char* fontname = "Segoe";

static inline void initFonts(const char *filename = "ui/font/font.tb.txt") {
	register_tbbf_font_renderer();
	register_stb_font_renderer();
	tb::g_font_manager->addFontInfo(filename, fontname);
}

static inline tb::TBFontFace *getFont(int dpSize = 14, bool registerAsDefault = false) {
	tb::TBFontFace *_font;
	tb::TBFontDescription fd;
	fd.setID(TBIDC(fontname));
	fd.setSize(tb::g_tb_skin->getDimensionConverter()->dpToPx(dpSize));

	tb::TBFontManager *fontMgr = tb::g_font_manager;
	if (registerAsDefault) {
		fontMgr->setDefaultFontDescription(fd);
	}

	if (fontMgr->hasFontFace(fd)) {
		_font = fontMgr->getFontFace(fd);
	} else {
		_font = fontMgr->createFontFace(fd);
	}
	core_assert_msg(_font != nullptr, "Could not find the default font - make sure the ui is already configured");
	_font->renderGlyphs(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ");
	return _font;
}

}
}
