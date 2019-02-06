/**
 * @file
 */

#pragma once

#include "TurboBadger.h"
#include "core/Common.h"

#ifdef TB_FONT_RENDERER_TBBF
extern void register_tbbf_font_renderer();
#endif
#ifdef TB_FONT_RENDERER_STB
extern void register_stb_font_renderer();
#endif

namespace ui {
namespace turbobadger {

static const char* fontname = "Segoe";

static inline void initFonts(const char *filename = "ui/font/font.tb.txt") {
#ifdef TB_FONT_RENDERER_TBBF
	register_tbbf_font_renderer();
#endif
#ifdef TB_FONT_RENDERER_STB
	register_stb_font_renderer();
#endif
	tb::g_font_manager->AddFontInfo(filename, fontname);
}

static inline tb::TBFontFace *getFont(int dpSize = 14, bool registerAsDefault = false) {
	tb::TBFontFace *_font;
	tb::TBFontDescription fd;
	fd.SetID(TBIDC(fontname));
	fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(dpSize));

	tb::TBFontManager *fontMgr = tb::g_font_manager;
	if (registerAsDefault) {
		fontMgr->SetDefaultFontDescription(fd);
	}

	if (fontMgr->HasFontFace(fd)) {
		_font = fontMgr->GetFontFace(fd);
	} else {
		_font = fontMgr->CreateFontFace(fd);
	}
	core_assert_msg(_font != nullptr, "Could not find the default font - make sure the ui is already configured");
	_font->RenderGlyphs(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ");
	return _font;
}

}
}
