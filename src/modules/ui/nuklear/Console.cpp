/**
 * @file
 */

#include "Console.h"
#include "Nuklear.h"
#include "NuklearApp.h"
#include "Nuklear.h"

namespace ui {
namespace nuklear {

Console::Console(NuklearApp* app) :
		Super(), _app(app) {
}

void Console::drawString(int x, int y, const glm::ivec4& color, int colorIndex, const char* str, int len) {
	struct nk_context* ctx = _app->context().ctx;
	const struct nk_user_font *font = &_app->font(_fontSize)->handle;
	struct nk_command_buffer* cmdBuf = nk_window_get_canvas(ctx);
	const int width = font->width(font->userdata, font->height, str, len);
	const struct nk_rect& rect = nk_rect(x, y, width, font->height);
	nk_draw_text(cmdBuf, rect, str,
			strlen(str), font, nk_rgba(0, 0, 0, 255), nk_rgba(color.r, color.g, color.b, color.a));
}

void Console::afterRender(const math::Rect<int> &rect) {
	struct nk_context* ctx = _app->context().ctx;
	nk_end(ctx);
}

void Console::beforeRender(const math::Rect<int> &rect) {
	struct nk_context* ctx = _app->context().ctx;
	const struct nk_rect nkrect{(float)rect.getMinX(), (float)rect.getMinZ(), (float)rect.getMaxX(), (float)rect.getMaxZ()};
	nk_begin(ctx, "in-game-console", nkrect, NK_WINDOW_NO_SCROLLBAR);
}

int Console::lineHeight() {
	const struct nk_user_font *font = &_app->font(_fontSize)->handle;
	const int lineHeight = font->height;
	return lineHeight;
}

glm::ivec2 Console::stringSize(const char* s, int length) {
	const struct nk_user_font *font = &_app->font(_fontSize)->handle;
	return glm::ivec2(font->width(font->userdata, font->height, s, length), lineHeight());
}

}
}
