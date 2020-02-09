/**
 * @file
 */

#include "tb_style_edit_content.h"
#include "core/Assert.h"
#include "tb_style_edit.h"

namespace tb {

int TBTextFragmentContentFactory::getContent(const char *text) {
	if (text[0] == '<') {
		int i = 0;
		while (text[i] != '>' && text[i] > 31) {
			i++;
		}
		if (text[i] == '>') {
			i++;
			return i;
		}
	}
	return 0;
}

TBTextFragmentContent *TBTextFragmentContentFactory::createFragmentContent(const char *text, int textLen) {
	if (SDL_strncmp(text, "<hr>", textLen) == 0) {
		return new TBTextFragmentContentHR(100, 2);
	}
	if (SDL_strncmp(text, "<u>", textLen) == 0) {
		return new TBTextFragmentContentUnderline();
	}
	if (SDL_strncmp(text, "<color ", Min(textLen, 7)) == 0) {
		TBColor color;
		color.setFromString(text + 7, textLen - 8);
		return new TBTextFragmentContentTextColor(color);
	}
	if (SDL_strncmp(text, "</", Min(textLen, 2)) == 0) {
		return new TBTextFragmentContentStylePop();
	}
	return nullptr;
}

TBTextFragmentContentHR::TBTextFragmentContentHR(int32_t widthInPercent, int32_t height)
	: width_in_percent(widthInPercent), height(height) {
}

void TBTextFragmentContentHR::paint(const TBPaintProps *props, TBTextFragment *fragment) {
	int x = props->translate_x + fragment->xpos;
	int y = props->translate_y + fragment->ypos;

	int w = props->block->styledit->layout_width * width_in_percent / 100;
	x += (props->block->styledit->layout_width - w) / 2;

	TBStyleEditListener *listener = props->block->styledit->listener;
	listener->drawRectFill(TBRect(x, y, w, height), props->props->data->text_color);
}

int32_t TBTextFragmentContentHR::getWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
	return Max(block->styledit->layout_width, 0);
}

int32_t TBTextFragmentContentHR::getHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
	return height;
}

void TBTextFragmentContentUnderline::paint(const TBPaintProps *props, TBTextFragment *fragment) {
	if (TBTextProps::Data *data = props->props->push())
		data->underline = true;
}

void TBTextFragmentContentTextColor::paint(const TBPaintProps *props, TBTextFragment *fragment) {
	if (TBTextProps::Data *data = props->props->push())
		data->text_color = color;
}

void TBTextFragmentContentStylePop::paint(const TBPaintProps *props, TBTextFragment *fragment) {
	props->props->pop();
}

} // namespace tb
