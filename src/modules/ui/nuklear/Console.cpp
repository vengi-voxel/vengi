/**
 * @file
 */

#include "Console.h"
#include "Nuklear.h"
#include "NuklearApp.h"

namespace ui {
namespace nuklear {

Console::Console(struct nk_context* ctx) :
		Super(), _ctx(ctx) {
}

void Console::drawString(int x, int y, const glm::ivec4& color, int colorIndex, const char* str, int len) {
	const struct nk_user_font *font = _ctx->style.font;
	struct nk_command_buffer* cmdBuf = nk_window_get_canvas(_ctx);
	const int width = font->width(font->userdata, font->height, str, len);
	const struct nk_rect& rect = nk_rect(x, y, width, font->height);
	nk_draw_text(cmdBuf, rect, str,
			strlen(str), font, nk_rgba(0, 0, 0, 255), nk_rgba(color.r, color.g, color.b, color.a));
}

void Console::afterRender(const math::Rect<int> &rect) {
	nk_end(_ctx);
}

void Console::beforeRender(const math::Rect<int> &rect) {
	const struct nk_rect nkrect{(float)rect.getMinX(), (float)rect.getMinZ(), (float)rect.getMaxX(), (float)rect.getMaxZ()};
	nk_begin(_ctx, "in-game-console", nkrect, NK_WINDOW_NO_SCROLLBAR);
}

int Console::lineHeight() {
	const struct nk_user_font *styleFont = _ctx->style.font;
	const int lineHeight = styleFont->height;
	return lineHeight;
}

glm::ivec2 Console::stringSize(const char* s, int length) {
	const struct nk_user_font *styleFont = _ctx->style.font;
	return glm::ivec2(styleFont->width(styleFont->userdata, styleFont->height, s, length), lineHeight());
}

}
}
