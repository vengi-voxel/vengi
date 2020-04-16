/**
 * @file
 */

#include "LUAFunctions.h"
#include "video/TexturePool.h"
#include "video/Texture.h"
#include "core/ArrayLength.h"
#include "video/Renderer.h"
#include "core/command/Command.h"
#include "LUAUIApp.h"
#include <SDL_stdinc.h>

namespace ui {
namespace nuklear {

constexpr int UILUA_MAX_RATIOS = 1024;
static float floatsArray[UILUA_MAX_RATIOS];
static int currentFloatsArrayIndex;

static inline struct nk_context* uilua_ctx(lua_State* s) {
	struct nk_context* ctx = lua::LUA::globalData<struct nk_context>(s, "context");
	core_assert(ctx != nullptr);
	return ctx;
}

static inline struct nkc_context* uilua_cctx(lua_State* s) {
	struct nkc_context* ctx = lua::LUA::globalData<struct nkc_context>(s, "ccontext");
	core_assert(ctx != nullptr);
	return ctx;
}

static inline video::TexturePool* uilua_texturepool(lua_State* s) {
	video::TexturePool* texturePool = lua::LUA::globalData<video::TexturePool>(s, "texturepool");
	core_assert(texturePool != nullptr);
	return texturePool;
}

static inline LUAUIApp* uilua_app(lua_State* s) {
	LUAUIApp* app = lua::LUA::globalData<LUAUIApp>(s, "app");
	core_assert(app != nullptr);
	return app;
}

struct Flags {
	const char *name;
	uint32_t flag;
};

static constexpr Flags windowFlags[] = {
	{ "scroll_auto_hide", NK_WINDOW_SCROLL_AUTO_HIDE },
	{ "minimizable", NK_WINDOW_MINIMIZABLE },
	{ "background", NK_WINDOW_BACKGROUND },
	{ "scalable", NK_WINDOW_SCALABLE },
	{ "closable", NK_WINDOW_CLOSABLE },
	{ "movable", NK_WINDOW_MOVABLE },
	{ "border", NK_WINDOW_BORDER },
	{ "title", NK_WINDOW_TITLE },
	{ nullptr, 0u }
};

static constexpr Flags symbolFlags[] = {
	{ "none", NK_SYMBOL_NONE },
	{ "x", NK_SYMBOL_X },
	{ "underscore", NK_SYMBOL_UNDERSCORE },
	{ "circle_solid", NK_SYMBOL_CIRCLE_SOLID },
	{ "circle_outline", NK_SYMBOL_CIRCLE_OUTLINE },
	{ "rect_solid", NK_SYMBOL_RECT_SOLID },
	{ "rect_outline", NK_SYMBOL_RECT_OUTLINE },
	{ "triangle_up", NK_SYMBOL_TRIANGLE_UP },
	{ "triangle_down", NK_SYMBOL_TRIANGLE_DOWN },
	{ "triangle_left", NK_SYMBOL_TRIANGLE_LEFT },
	{ "triangle_right", NK_SYMBOL_TRIANGLE_RIGHT },
	{ "plus", NK_SYMBOL_PLUS },
	{ "minus", NK_SYMBOL_MINUS },
	{ "max", NK_SYMBOL_MAX },
	{ nullptr, 0u }
};

static constexpr Flags alignFlags[] = {
	{ "left", NK_TEXT_LEFT },
	{ "centered", NK_TEXT_CENTERED },
	{ "right", NK_TEXT_RIGHT },
	{ "top_left", NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_LEFT },
	{ "top_centered", NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED },
	{ "top_right", NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_RIGHT },
	{ "bottom_left", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_LEFT },
	{ "bottom_centered", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED },
	{ "bottom_right", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_RIGHT },
	{ nullptr, 0u }
};

static constexpr Flags layoutFlags[] = {
	{ "dynamic", NK_DYNAMIC },
	{ "static", NK_STATIC },
	{ nullptr, 0u }
};

static constexpr Flags treeFlags[] = {
	{ "node", NK_TREE_NODE },
	{ "tab", NK_TREE_TAB },
	{ nullptr, 0u }
};

static constexpr Flags stateFlags[] = {
	{ "collapsed", NK_MINIMIZED },
	{ "expanded", NK_MAXIMIZED },
	{ nullptr, 0u }
};

static constexpr Flags behaviorFlags[] = {
	{ "default", NK_BUTTON_DEFAULT },
	{ "repeater", NK_BUTTON_REPEATER },
	{ nullptr, 0u }
};

static constexpr Flags colorFormatFlags[] = {
	{ "RGB", NK_RGB },
	{ "RGBA", NK_RGBA },
	{ nullptr, 0u }
};

static constexpr Flags editTypeFlags[] = {
	{ "simple", NK_EDIT_SIMPLE },
	{ "field", NK_EDIT_FIELD },
	{ "box", NK_EDIT_BOX },
	{ "editor", NK_EDIT_EDITOR },
	{ nullptr, 0u }
};

static constexpr Flags popupFlags[] = {
	{ "dynamic", NK_POPUP_DYNAMIC },
	{ "static", NK_POPUP_STATIC },
	{ nullptr, 0u }
};

static void uilua_color(int r, int g, int b, int a, char *colorString, size_t colorStringSize) {
	r = NK_CLAMP(0, r, 255);
	g = NK_CLAMP(0, g, 255);
	b = NK_CLAMP(0, b, 255);
	a = NK_CLAMP(0, a, 255);
	if (a < 255) {
		SDL_snprintf(colorString, colorStringSize, "#%02x%02x%02x%02x", r, g, b, a);
	} else {
		SDL_snprintf(colorString, colorStringSize, "#%02x%02x%02x", r, g, b);
	}
}

static int uilua_is_active(struct nk_context *ctx) {
	if (!ctx) {
		return 0;
	}
	struct nk_window *iter = ctx->begin;
	while (iter != nullptr) {
		/* check if window is being hovered */
		if ((iter->flags & NK_WINDOW_MINIMIZED) != 0) {
			struct nk_rect header = iter->bounds;
			header.h = ctx->style.font->height + 2 * ctx->style.window.header.padding.y;
			if (nk_input_is_mouse_hovering_rect(&ctx->input, header)) {
				return 1;
			}
		} else if (nk_input_is_mouse_hovering_rect(&ctx->input, iter->bounds)) {
			return 1;
		}
		/* check if window popup is being hovered */
		if (iter->popup.active && iter->popup.win && nk_input_is_mouse_hovering_rect(&ctx->input, iter->popup.win->bounds)) {
			return 1;
		}
		if (iter->edit.active & NK_EDIT_ACTIVE) {
			return 1;
		}
		iter = iter->next;
	}
	return 0;
}

static inline int uilua_is_hex(char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int uilua_is_color(lua_State *s, int index) {
	if (index < 0) {
		index += lua_gettop(s) + 1;
	}
	if (!lua_isstring(s, index)) {
		return 0;
	}
	size_t len;
	const char *color_string = lua_tolstring(s, index, &len);
	if ((len == 7 || len == 9) && color_string[0] == '#') {
		for (size_t i = 1; i < len; ++i) {
			if (!uilua_is_hex(color_string[i])) {
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

static struct nk_image uilua_checkImage(lua_State *s, int index) {
	if (index < 0) {
		index += lua_gettop(s) + 1;
	}
	const char* imageName = luaL_checkstring(s, index);
	video::TexturePool* texturePool = uilua_texturepool(s);
	clua_assert(s, texturePool != nullptr, "Could not get texture pool: '%s'");
	const video::TexturePtr& tex = texturePool->load(imageName);
	clua_assert(s, tex != nullptr, "Could not load image: '%s'");
	const video::Id handle = tex->handle();
	const int width = tex->width();
	const int height = tex->height();
	struct nk_image image;
	image.handle = nk_handle_id(handle);
	image.w = width;
	image.h = height;
	image.region[0] = 0;
	image.region[1] = 0;
	image.region[2] = image.region[0] + width;
	image.region[3] = image.region[1] + height;
	return image;
}

static struct nk_color uilua_checkcolor(lua_State *s, int index) {
	if (index < 0) {
		index += lua_gettop(s) + 1;
	}
	if (!uilua_is_color(s, index)) {
		if (lua_isstring(s, index)) {
			const char *msg = lua_pushfstring(s, "bad color string '%s'", lua_tostring(s, index));
			luaL_argerror(s, index, msg);
		} else {
			clua_typerror(s, index, "color string");
		}
	}
	size_t len;
	const char *colorString = lua_tolstring(s, index, &len);
	int r = 0, g = 0, b = 0, a = 255;
	if (SDL_sscanf(colorString, "#%02x%02x%02x", &r, &g, &b) != 3) {
		clua_typerror(s, index, "color string without rgb");
	}
	if (len == 9) {
		if (SDL_sscanf(colorString + 7, "%02x", &a) != 1) {
			clua_typerror(s, index, "color string without alpha");
		}
	}
	struct nk_color color = { (nk_byte)r, (nk_byte)g, (nk_byte)b, (nk_byte)a };
	return color;
}

static struct nk_colorf uilua_checkcolorf(lua_State *s, int index) {
	const struct nk_color color = uilua_checkcolor(s, index);
	struct nk_colorf colorf = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
	return colorf;
}

/**
 * @brief Convert the given window flags to nuklear enum values
 */
static nk_flags uilua_window_flag(lua_State *s, int argsStartIndex) {
	const int argc = lua_gettop(s);
	nk_flags flags = NK_WINDOW_NO_SCROLLBAR;
	for (int i = argsStartIndex; i <= argc; ++i) {
		const char *flagId = luaL_checkstring(s, i);
		if (!SDL_strcmp(flagId, "scrollbar")) {
			flags &= ~NK_WINDOW_NO_SCROLLBAR;
			continue;
		}
		const Flags* flag = windowFlags;
		for (; flag->name != nullptr; ++flag) {
			if (!SDL_strcmp(flag->name, flagId)) {
				flags |= flag->flag;
				break;
			}
		}
		if (flag->name == nullptr) {
			const char *msg = lua_pushfstring(s, "Unknown window flag given: '%s'", flagId);
			return luaL_argerror(s, i, msg);
		}
	}
	return flags;
}

static uint32_t uilua_checkflag(lua_State *s, int index, const Flags* flags) {
	if (index < 0) {
		index += lua_gettop(s) + 1;
	}
	const char *flagId = luaL_checkstring(s, index);
	for (const Flags* flag = flags; flag->name != nullptr; ++flag) {
		if (!SDL_strcmp(flag->name, flagId)) {
			return flag->flag;
		}
	}
	const char *msg = lua_pushfstring(s, "Unknown flag given: '%s'", flagId);
	return luaL_argerror(s, index, msg);
}

static inline nk_flags uilua_checkalign(lua_State *s, int index) {
	return (nk_flags)uilua_checkflag(s, index, alignFlags);
}

static inline nk_symbol_type uilua_checksymbol(lua_State *s, int index) {
	return (nk_symbol_type)uilua_checkflag(s, index, symbolFlags);
}

static inline nk_layout_format uilua_checkformat(lua_State *s, int index) {
	return (nk_layout_format)uilua_checkflag(s, index, layoutFlags);
}

static inline nk_tree_type uilua_checktree(lua_State *s, int index) {
	return (nk_tree_type)uilua_checkflag(s, index, treeFlags);
}

static inline nk_collapse_states uilua_checkstate(lua_State *s, int index) {
	return (nk_collapse_states)uilua_checkflag(s, index, stateFlags);
}

static inline nk_button_behavior uilua_checkbehavior(lua_State *s, int index) {
	return (nk_button_behavior)uilua_checkflag(s, index, behaviorFlags);
}

static inline nk_color_format uilua_checkcolorformat(lua_State *s, int index) {
	return (nk_color_format)uilua_checkflag(s, index, colorFormatFlags);
}

static inline nk_flags uilua_checkedittype(lua_State *s, int index) {
	return (nk_flags)uilua_checkflag(s, index, editTypeFlags);
}

static inline nk_popup_type uilua_checkpopup(lua_State *s, int index) {
	return (nk_popup_type)uilua_checkflag(s, index, popupFlags);
}

static int uilua_bounds(lua_State *s, int n, struct nk_rect& rect) {
	const int argc = lua_gettop(s);
	bool noPositionGiven;
	if (argc == 2) {
		// if there are just two arguments given, the position is the center
		// of the screen
		noPositionGiven = true;
	} else {
		noPositionGiven = !lua_isnumber(s, n + 2);
	}
	if (noPositionGiven) {
		rect.w = luaL_checknumber(s, n + 0);
		rect.h = luaL_checknumber(s, n + 1);
		int x, y, w, h;
		video::getViewport(x, y, w, h);
		rect.x = w / 2 - rect.w / 2;
		rect.y = h / 2 - rect.h / 2;
		return 2;
	}
	rect.x = luaL_checknumber(s, n + 0);
	rect.y = luaL_checknumber(s, n + 1);
	rect.w = luaL_checknumber(s, n + 2);
	rect.h = luaL_checknumber(s, n + 3);
	return 4;
}

int uilua_window_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const char *name;
	const char *title;
	int startIndex;
	const bool noNameDefined = lua_isnumber(s, 2);
	if (noNameDefined) {
		name = title = luaL_checkstring(s, 1);
		startIndex = 2;
	} else {
		name = luaL_checkstring(s, 1);
		title = luaL_checkstring(s, 2);
		startIndex = 3;
	}
	struct nk_rect bounds;
	startIndex += uilua_bounds(s, startIndex, bounds);
	const nk_flags flags = uilua_window_flag(s, startIndex);
	const int retVal = nk_begin_titled(ctx, name, title, bounds, flags);
	lua_pushboolean(s, retVal);
	return 1;
}

int uilua_window_end(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	nk_end(ctx);
	return 0;
}

int uilua_window_get_bounds(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_rect& rect = nk_window_get_bounds(ctx);
	lua_pushnumber(s, rect.x);
	lua_pushnumber(s, rect.y);
	lua_pushnumber(s, rect.w);
	lua_pushnumber(s, rect.h);
	return 4;
}

int uilua_window_get_position(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_vec2& pos = nk_window_get_position(ctx);
	lua_pushnumber(s, pos.x);
	lua_pushnumber(s, pos.y);
	return 2;
}

int uilua_window_get_size(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_vec2& size = nk_window_get_size(ctx);
	lua_pushnumber(s, size.x);
	lua_pushnumber(s, size.y);
	return 2;
}

int uilua_window_get_content_region(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_rect& rect = nk_window_get_content_region(ctx);
	lua_pushnumber(s, rect.x);
	lua_pushnumber(s, rect.y);
	lua_pushnumber(s, rect.w);
	lua_pushnumber(s, rect.h);
	return 4;
}

int uilua_button(lua_State *s) {
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1 && argc <= 2);
	struct nk_context* ctx = uilua_ctx(s);
	const char *title = nullptr;
	if (!lua_isnil(s, 1)) {
		title = luaL_checkstring(s, 1);
	}
	bool useColor = false;
	bool useImage = false;
	struct nk_color color;
	nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	if (argc >= 2 && !lua_isnil(s, 2)) {
		if (lua_isstring(s, 2)) {
			if (uilua_is_color(s, 2)) {
				color = uilua_checkcolor(s, 2);
				useColor = true;
			} else {
				symbol = uilua_checksymbol(s, 2);
			}
		} else {
			image = uilua_checkImage(s, 1);
			useImage = true;
		}
	}
	nk_flags align = ctx->style.button.text_alignment;
	int activated = 0;
	if (title != nullptr) {
		if (useColor) {
			clua_assert(s, false, "%s: color buttons can't have titles");
		} else if (symbol != NK_SYMBOL_NONE) {
			activated = nk_button_symbol_label(ctx, symbol, title, align);
		} else if (useImage) {
			activated = nk_button_image_label(ctx, image, title, align);
		} else {
			activated = nk_button_label(ctx, title);
		}
	} else {
		if (useColor) {
			activated = nk_button_color(ctx, color);
		} else if (symbol != NK_SYMBOL_NONE) {
			activated = nk_button_symbol(ctx, symbol);
		} else if (useImage) {
			activated = nk_button_image(ctx, image);
		} else {
			clua_assert(s, false, "%s: must specify a title, color, symbol, and/or image");
		}
	}
	lua_pushboolean(s, activated);
	return 1;
}

int uilua_push_scissor(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 4);
	struct nk_context* ctx = uilua_ctx(s);
	const float x = luaL_checknumber(s, 1);
	const float y = luaL_checknumber(s, 2);
	const float w = luaL_checknumber(s, 3);
	const float h = luaL_checknumber(s, 4);
	nk_push_scissor(&ctx->current->buffer, nk_rect(x, y, w, h));
	return 0;
}

int uilua_label(lua_State *s) {
	int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1 && argc <= 3);
	struct nk_context* ctx = uilua_ctx(s);
	const char *text = luaL_checkstring(s, 1);
	nk_flags align = NK_TEXT_LEFT;
	bool wrap = false;
	struct nk_color color;
	bool useColor = false;
	if (argc >= 2) {
		const char *alignString = luaL_checkstring(s, 2);
		if (!SDL_strcmp(alignString, "wrap")) {
			wrap = true;
		} else {
			align = uilua_checkalign(s, 2);
		}
		if (argc >= 3) {
			color = uilua_checkcolor(s, 3);
			useColor = true;
		}
	}
	if (useColor) {
		if (wrap) {
			nk_label_colored_wrap(ctx, text, color);
		} else {
			nk_label_colored(ctx, text, align, color);
		}
	} else {
		if (wrap) {
			nk_label_wrap(ctx, text);
		} else {
			nk_label(ctx, text, align);
		}
	}
	return 0;
}

int uilua_text(lua_State *s) {
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1);
	struct nkc_context* cctx = uilua_cctx(s);
	struct nk_context* ctx = cctx->ctx;
	LUAUIApp* app = uilua_app(s);
	const char *text = luaL_checkstring(s, 1);

	const struct nk_style *style = &ctx->style;
	struct nk_color color = style->text.color;
	nk_flags alignment = NK_TEXT_LEFT;
	const struct nk_user_font *font = style->font;

	if (argc == 2) {
		if (!lua_istable(s, 2)) {
			clua_typerror(s, 2, "table");
		}
		lua_pushnil(s);
		while (lua_next(s, 2) != 0) {
			const char *field = luaL_checkstring(s, -2);
			if (!SDL_strcmp(field, "color")) {
				color = uilua_checkcolor(s, -1);
			} else if (!SDL_strcmp(field, "size")) {
				font = &app->font(luaL_checkinteger(s, -1))->handle;
			} else if (!SDL_strcmp(field, "align")) {
				const char *align = luaL_checkstring(s, -1);
				if (!SDL_strcmp(align, "left")) {
					alignment = NK_TEXT_LEFT;
				} else if (!SDL_strcmp(align, "right")) {
					alignment = NK_TEXT_RIGHT;
				} else if (!SDL_strcmp(align, "center")) {
					alignment = NK_TEXT_CENTERED;
				}
			} else {
				clua_assert(s, false, "Unknown field given");
			}
			lua_pop(s, 1);
		}
	}

	nkc_text(cctx, text, alignment, color, font);
	return 0;
}

int uilua_image(lua_State *s) {
	const int argc = lua_gettop(s);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_image& image = uilua_checkImage(s, 1);
	if (argc == 1) {
		nk_image(ctx, image);
		return 0;
	}
	clua_assert_argc(s, argc == 5);

	const float x = luaL_checknumber(s, 2);
	const float y = luaL_checknumber(s, 3);
	const float w = luaL_checknumber(s, 4);
	const float h = luaL_checknumber(s, 5);
	static constexpr struct nk_color white = {255, 255, 255, 255};
	nk_draw_image(&ctx->current->buffer, nk_rect(x, y, w, h), &image, white);
	return 0;
}

int uilua_window_has_focus(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	const int hasFocus = nk_window_has_focus(ctx);
	lua_pushboolean(s, hasFocus);
	return 1;
}

int uilua_window_is_collapsed(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	const int isCollapsed = nk_window_is_collapsed(ctx, name);
	lua_pushboolean(s, isCollapsed);
	return 1;
}

int uilua_window_is_hidden(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	const int isHidden = nk_window_is_hidden(ctx, name);
	lua_pushboolean(s, isHidden);
	return 1;
}

int uilua_window_is_active(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	const int isActive = nk_window_is_active(ctx, name);
	lua_pushboolean(s, isActive);
	return 1;
}

int uilua_window_is_hovered(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	const int isHovered = nk_window_is_hovered(ctx);
	lua_pushboolean(s, isHovered);
	return 1;
}

int uilua_window_is_any_hovered(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	const int isAnyHovered = nk_window_is_any_hovered(ctx);
	lua_pushboolean(s, isAnyHovered);
	return 1;
}

int uilua_item_is_any_active(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	lua_pushboolean(s, uilua_is_active(ctx));
	return 1;
}

int uilua_window_set_bounds(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 5);
	const char *name = luaL_checkstring(s, 1);
	struct nk_rect bounds;
	bounds.x = luaL_checknumber(s, 2);
	bounds.y = luaL_checknumber(s, 3);
	bounds.w = luaL_checknumber(s, 4);
	bounds.h = luaL_checknumber(s, 5);
	nk_window_set_bounds(ctx, name, bounds);
	return 0;
}

int uilua_window_set_position(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 3);
	const char *name = luaL_checkstring(s, 1);
	struct nk_vec2 pos;
	pos.x = luaL_checknumber(s, 2);
	pos.y = luaL_checknumber(s, 3);
	nk_window_set_position(ctx, name, pos);
	return 0;
}

int uilua_window_set_size(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 3);
	const char *name = luaL_checkstring(s, 1);
	struct nk_vec2 size;
	size.x = luaL_checknumber(s, 2);
	size.y = luaL_checknumber(s, 3);
	nk_window_set_size(ctx, name, size);
	return 0;
}

int uilua_window_set_focus(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	nk_window_set_focus(ctx, name);
	return 0;
}

int uilua_window_close(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	nk_window_close(ctx, name);
	return 0;
}

int uilua_window_collapse(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	nk_window_collapse(ctx, name, NK_MINIMIZED);
	return 0;
}

int uilua_window_expand(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	nk_window_collapse(ctx, name, NK_MAXIMIZED);
	return 0;
}

int uilua_window_show(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	nk_window_show(ctx, name, NK_SHOWN);
	return 0;
}

int uilua_window_hide(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *name = luaL_checkstring(s, 1);
	nk_window_show(ctx, name, NK_HIDDEN);
	return 0;
}

int uilua_layout_row(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 3 && argc <= 4);
	const nk_layout_format format = uilua_checkformat(s, 1);
	const float height = luaL_checknumber(s, 2);
	bool useRatios = false;
	if (format == NK_DYNAMIC) {
		clua_assert_argc(s, argc == 3);
		if (lua_isnumber(s, 3)) {
			const int cols = luaL_checkinteger(s, 3);
			nk_layout_row_dynamic(ctx, height, cols);
		} else {
			if (!lua_istable(s, 3)) {
				luaL_argerror(s, 3, "should be a number or table");
			}
			useRatios = true;
		}
	} else if (format == NK_STATIC) {
		if (argc == 4) {
			const int itemWidth = luaL_checkinteger(s, 3);
			const int cols = luaL_checkinteger(s, 4);
			nk_layout_row_static(ctx, height, itemWidth, cols);
		} else {
			if (!lua_istable(s, 3)) {
				luaL_argerror(s, 3, "should be a number or table");
			}
			useRatios = true;
		}
	}
	if (useRatios) {
		const int cols = lua_objlen(s, -1);
		clua_assert(s, cols < lengthof(floatsArray), "Overflow for ratios: '%s'");

		if (currentFloatsArrayIndex + cols >= lengthof(floatsArray)) {
			currentFloatsArrayIndex = 0;
		}

		int j = currentFloatsArrayIndex;
		core_assert_always(j + cols < lengthof(floatsArray));
		for (int i = 1; i <= cols; ++i, ++j) {
			lua_rawgeti(s, -1, i);
			if (!lua_isnumber(s, -1)) {
				luaL_argerror(s, lua_gettop(s) - 1, "should contain numbers only");
			}
			floatsArray[j] = lua_tonumber(s, -1);
			lua_pop(s, 1);
		}
		nk_layout_row(ctx, format, height, cols, floatsArray + currentFloatsArrayIndex);
		currentFloatsArrayIndex += cols;
	}
	return 0;
}

int uilua_layout_row_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 3);
	const nk_layout_format format = uilua_checkformat(s, 1);
	const float height = luaL_checknumber(s, 2);
	const int cols = luaL_checkinteger(s, 3);
	nk_layout_row_begin(ctx, format, height, cols);
	return 0;
}

int uilua_layout_row_push(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const float value = luaL_checknumber(s, 1);
	nk_layout_row_push(ctx, value);
	return 0;
}

int uilua_layout_row_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_layout_row_end(ctx);
	return 0;
}

int uilua_layout_space_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 3);
	const nk_layout_format format = uilua_checkformat(s, 1);
	const float height = luaL_checknumber(s, 2);
	const int widgetCount = luaL_checkinteger(s, 3);
	nk_layout_space_begin(ctx, format, height, widgetCount);
	return 0;
}

int uilua_layout_space_push(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 4);
	const float x = luaL_checknumber(s, 1);
	const float y = luaL_checknumber(s, 2);
	const float width = luaL_checknumber(s, 3);
	const float height = luaL_checknumber(s, 4);
	nk_layout_space_push(ctx, nk_rect(x, y, width, height));
	return 0;
}

int uilua_layout_space_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_layout_space_end(ctx);
	return 0;
}

int uilua_layout_space_bounds(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	const struct nk_rect& bounds = nk_layout_space_bounds(ctx);
	lua_pushnumber(s, bounds.x);
	lua_pushnumber(s, bounds.y);
	lua_pushnumber(s, bounds.w);
	lua_pushnumber(s, bounds.h);
	return 4;
}

int uilua_layout_space_to_screen(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 2);
	struct nk_vec2 local;
	local.x = luaL_checknumber(s, 1);
	local.y = luaL_checknumber(s, 2);
	struct nk_vec2 screen = nk_layout_space_to_screen(ctx, local);
	lua_pushnumber(s, screen.x);
	lua_pushnumber(s, screen.y);
	return 2;
}

int uilua_layout_space_to_local(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 2);
	struct nk_vec2 screen;
	screen.x = luaL_checknumber(s, 1);
	screen.y = luaL_checknumber(s, 2);
	struct nk_vec2 local = nk_layout_space_to_local(ctx, screen);
	lua_pushnumber(s, local.x);
	lua_pushnumber(s, local.y);
	return 2;
}

int uilua_layout_space_rect_to_screen(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 4);
	struct nk_rect local;
	local.x = luaL_checknumber(s, 1);
	local.y = luaL_checknumber(s, 2);
	local.w = luaL_checknumber(s, 3);
	local.h = luaL_checknumber(s, 4);
	struct nk_rect screen = nk_layout_space_rect_to_screen(ctx, local);
	lua_pushnumber(s, screen.x);
	lua_pushnumber(s, screen.y);
	lua_pushnumber(s, screen.w);
	lua_pushnumber(s, screen.h);
	return 4;
}

int uilua_layout_space_rect_to_local(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 4);
	struct nk_rect screen;
	screen.x = luaL_checknumber(s, 1);
	screen.y = luaL_checknumber(s, 2);
	screen.w = luaL_checknumber(s, 3);
	screen.h = luaL_checknumber(s, 4);
	struct nk_rect local = nk_layout_space_rect_to_screen(ctx, screen);
	lua_pushnumber(s, local.x);
	lua_pushnumber(s, local.y);
	lua_pushnumber(s, local.w);
	lua_pushnumber(s, local.h);
	return 4;
}

int uilua_layout_ratio_from_pixel(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const float pixelWidth = luaL_checknumber(s, 1);
	const float ratio = nk_layout_ratio_from_pixel(ctx, pixelWidth);
	lua_pushnumber(s, ratio);
	return 1;
}

int uilua_group_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) >= 1);
	const char *title = luaL_checkstring(s, 1);
	const nk_flags flags = uilua_window_flag(s, 2);
	const int open = nk_group_begin(ctx, title, flags);
	lua_pushboolean(s, open);
	return 1;
}

int uilua_group_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_group_end(ctx);
	return 0;
}

int uilua_tree_push(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 2 && argc <= 4);
	const nk_tree_type type = uilua_checktree(s, 1);
	const char *title = luaL_checkstring(s, 2);
	struct nk_image image;
	bool useImage = false;
	if (argc >= 3 && !lua_isnil(s, 3)) {
		image = uilua_checkImage(s, 3);
		useImage = true;
	}
	nk_collapse_states state = NK_MINIMIZED;
	if (argc >= 4) {
		state = uilua_checkstate(s, 4);
	}
	lua_Debug ar;
	lua_getstack(s, 1, &ar);
	lua_getinfo(s, "l", &ar);
	const int id = ar.currentline;
	int open = 0;
	if (useImage) {
		open = nk_tree_image_push_hashed(ctx, type, image, title, state, title, SDL_strlen(title), id);
	} else {
		open = nk_tree_push_hashed(ctx, type, title, state, title, SDL_strlen(title), id);
	}
	lua_pushboolean(s, open);
	return 1;
}

int uilua_tree_pop(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_tree_pop(ctx);
	return 0;
}

int uilua_button_set_behavior(lua_State *s) {
	clua_assert_argc(s, lua_gettop(s) == 1);
	struct nk_context* ctx = uilua_ctx(s);
	const nk_button_behavior behavior = uilua_checkbehavior(s, 1);
	nk_button_set_behavior(ctx, behavior);
	return 0;
}

int uilua_button_push_behavior(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	nk_button_behavior behavior = uilua_checkbehavior(s, 1);
	nk_button_push_behavior(ctx, behavior);
	return 0;
}

int uilua_button_pop_behavior(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_button_pop_behavior(ctx);
	return 0;
}

int uilua_checkbox(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 2);
	const char *text = luaL_checkstring(s, 1);
	if (lua_isboolean(s, 2)) {
		int value = lua_toboolean(s, 2);
		value = nk_check_label(ctx, text, value);
		lua_pushboolean(s, value);
	} else if (lua_istable(s, 2)) {
		lua_getfield(s, 2, "value");
		int value = lua_toboolean(s, -1);
		int changed = nk_checkbox_label(ctx, text, &value);
		if (changed) {
			lua_pushboolean(s, value);
			lua_setfield(s, 2, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, 2, "boolean or table");
	}
	return 1;
}

int uilua_radio(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc == 2 || argc == 3);
	const char *name = luaL_checkstring(s, 1);
	const char *text;
	if (argc == 3) {
		text = luaL_checkstring(s, 2);
	} else {
		text = luaL_checkstring(s, 1);
	}
	if (lua_isstring(s, -1)) {
		const char *value = lua_tostring(s, -1);
		int active = !SDL_strcmp(value, name);
		active = nk_option_label(ctx, text, active);
		if (active) {
			lua_pushstring(s, name);
		} else {
			lua_pushstring(s, value);
		}
	} else if (lua_istable(s, -1)) {
		lua_getfield(s, -1, "value");
		if (!lua_isstring(s, -1)) {
			luaL_argerror(s, argc, "should have a string value");
		}
		const char *value = lua_tostring(s, -1);
		int active = !SDL_strcmp(value, name);
		int changed = nk_radio_label(ctx, text, &active);
		if (changed && active) {
			lua_pushstring(s, name);
			lua_setfield(s, -3, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, argc, "string or table");
	}
	return 1;
}

int uilua_selectable(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 2 && argc <= 4);
	const char *text = luaL_checkstring(s, 1);
	struct nk_image image;
	bool useImage = false;
	if (argc >= 3 && !lua_isnil(s, 2)) {
		image = uilua_checkImage(s, 2);
		useImage = true;
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 4) {
		align = uilua_checkalign(s, 3);
	}
	if (lua_isboolean(s, -1)) {
		int value = lua_toboolean(s, -1);
		if (useImage) {
			value = nk_select_image_label(ctx, image, text, align, value);
		} else {
			value = nk_select_label(ctx, text, align, value);
		}
		lua_pushboolean(s, value);
	} else if (lua_istable(s, -1)) {
		lua_getfield(s, -1, "value");
		if (!lua_isboolean(s, -1)) {
			luaL_argerror(s, argc, "should have a boolean value");
		}
		int value = lua_toboolean(s, -1);
		int changed;
		if (useImage) {
			changed = nk_selectable_image_label(ctx, image, text, align, &value);
		} else {
			changed = nk_selectable_label(ctx, text, align, &value);
		}
		if (changed) {
			lua_pushboolean(s, value);
			lua_setfield(s, -3, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, argc, "boolean or table");
	}
	return 1;
}

int uilua_slider(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 4);
	const float min = luaL_checknumber(s, 1);
	const float max = luaL_checknumber(s, 3);
	const float step = luaL_checknumber(s, 4);
	if (lua_isnumber(s, 2)) {
		float value = lua_tonumber(s, 2);
		value = nk_slide_float(ctx, min, value, max, step);
		lua_pushnumber(s, value);
	} else if (lua_istable(s, 2)) {
		lua_getfield(s, 2, "value");
		if (!lua_isnumber(s, -1)) {
			luaL_argerror(s, 2, "should have a number value");
		}
		float value = lua_tonumber(s, -1);
		const int changed = nk_slider_float(ctx, min, &value, max, step);
		if (changed) {
			lua_pushnumber(s, value);
			lua_setfield(s, 2, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, 2, "number or table");
	}
	return 1;
}

int uilua_progress(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc == 2 || argc == 3);
	nk_size max = (nk_size)luaL_checknumber(s, 2);
	int modifiable = 0;
	if (argc == 3 && !lua_isnil(s, 3)) {
		modifiable = clua_checkboolean(s, 3);
	}
	if (lua_isnumber(s, 1)) {
		nk_size value = lua_tonumber(s, 1);
		value = nk_prog(ctx, value, max, modifiable);
		lua_pushnumber(s, value);
	} else if (lua_istable(s, 1)) {
		lua_getfield(s, 1, "value");
		if (!lua_isnumber(s, -1)) {
			luaL_argerror(s, 1, "should have a number value");
		}
		nk_size value = (nk_size) lua_tonumber(s, -1);
		const int changed = nk_progress(ctx, &value, max, modifiable);
		if (changed) {
			lua_pushnumber(s, value);
			lua_setfield(s, 1, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, 1, "number or table");
	}
	return 1;
}

int uilua_color_picker(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1 && argc <= 2);
	nk_color_format format = NK_RGB;
	if (argc >= 2) {
		format = uilua_checkcolorformat(s, 2);
	}
	if (lua_isstring(s, 1)) {
		struct nk_colorf color = uilua_checkcolorf(s, 1);
		color = nk_color_picker(ctx, color, format);
		char newColorString[10];
		uilua_color((int) (color.r * 255), (int) (color.g * 255),
				(int) (color.b * 255), (int) (color.a * 255), newColorString, sizeof(newColorString));
		lua_pushstring(s, newColorString);
	} else if (lua_istable(s, 1)) {
		lua_getfield(s, 1, "value");
		if (!uilua_is_color(s, -1)) {
			luaL_argerror(s, 1, "should have a color string value");
		}
		struct nk_colorf color = uilua_checkcolorf(s, -1);
		const int changed = nk_color_pick(ctx, &color, format);
		if (changed) {
			char newColorString[10];
			uilua_color((int) (color.r * 255), (int) (color.g * 255),
				(int) (color.b * 255), (int) (color.a * 255), newColorString, sizeof(newColorString));
			lua_pushstring(s, newColorString);
			lua_setfield(s, 1, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, 1, "string or table");
	}
	return 1;
}

int uilua_property(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 6);
	const char *name = luaL_checkstring(s, 1);
	double min = luaL_checknumber(s, 2);
	double max = luaL_checknumber(s, 4);
	double step = luaL_checknumber(s, 5);
	float inc_per_pixel = luaL_checknumber(s, 6);
	if (lua_isnumber(s, 3)) {
		double value = lua_tonumber(s, 3);
		value = nk_propertyd(ctx, name, min, value, max, step, inc_per_pixel);
		lua_pushnumber(s, value);
	} else if (lua_istable(s, 3)) {
		lua_getfield(s, 3, "value");
		if (!lua_isnumber(s, -1)) {
			luaL_argerror(s, 3, "should have a number value");
		}
		double value = lua_tonumber(s, -1);
		double old = value;
		nk_property_double(ctx, name, min, &value, max, step, inc_per_pixel);
		int changed = value != old;
		if (changed) {
			lua_pushnumber(s, value);
			lua_setfield(s, 3, "value");
		}
		lua_pushboolean(s, changed);
	} else if (clua_istype<glm::vec4>(s, 3)) {
		glm::vec4* v = clua_get<glm::vec4>(s, 3);
		v->x = nk_propertyd(ctx, name, min, v->x, max, step, inc_per_pixel);
		v->y = nk_propertyd(ctx, name, min, v->y, max, step, inc_per_pixel);
		v->z = nk_propertyd(ctx, name, min, v->z, max, step, inc_per_pixel);
		v->w = nk_propertyd(ctx, name, min, v->w, max, step, inc_per_pixel);
		clua_push(s, *v);
	} else if (clua_istype<glm::vec3>(s, 3)) {
		glm::vec3* v = clua_get<glm::vec3>(s, 3);
		v->x = nk_propertyd(ctx, name, min, v->x, max, step, inc_per_pixel);
		v->y = nk_propertyd(ctx, name, min, v->y, max, step, inc_per_pixel);
		v->z = nk_propertyd(ctx, name, min, v->z, max, step, inc_per_pixel);
		clua_push(s, *v);
	} else if (clua_istype<glm::vec2>(s, 3)) {
		glm::vec2* v = clua_get<glm::vec2>(s, 3);
		v->x = nk_propertyd(ctx, name, min, v->x, max, step, inc_per_pixel);
		v->y = nk_propertyd(ctx, name, min, v->y, max, step, inc_per_pixel);
		clua_push(s, *v);
	} else if (clua_istype<glm::ivec4>(s, 3)) {
		glm::ivec4* v = clua_get<glm::ivec4>(s, 3);
		v->x = nk_propertyi(ctx, name, min, v->x, max, step, inc_per_pixel);
		v->y = nk_propertyi(ctx, name, min, v->y, max, step, inc_per_pixel);
		v->z = nk_propertyi(ctx, name, min, v->z, max, step, inc_per_pixel);
		v->w = nk_propertyi(ctx, name, min, v->w, max, step, inc_per_pixel);
		clua_push(s, *v);
	} else if (clua_istype<glm::ivec3>(s, 3)) {
		glm::ivec3* v = clua_get<glm::ivec3>(s, 3);
		v->x = nk_propertyi(ctx, name, min, v->x, max, step, inc_per_pixel);
		v->y = nk_propertyi(ctx, name, min, v->y, max, step, inc_per_pixel);
		v->z = nk_propertyi(ctx, name, min, v->z, max, step, inc_per_pixel);
		clua_push(s, *v);
	} else if (clua_istype<glm::ivec2>(s, 3)) {
		glm::ivec2* v = clua_get<glm::ivec2>(s, 3);
		v->x = nk_propertyi(ctx, name, min, v->x, max, step, inc_per_pixel);
		v->y = nk_propertyi(ctx, name, min, v->y, max, step, inc_per_pixel);
		clua_push(s, *v);
	} else {
		clua_typerror(s, 3, "number, vector or table");
	}
	return 1;
}

int uilua_model(lua_State *s) {
	struct nkc_context* cctx = uilua_cctx(s);
	const LUAUIApp* app = uilua_app(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1);
	struct nkc_model model;
	model.timeSeconds = app->nowSeconds();
	model.modelPath = luaL_checkstring(s, 1);
	model.camera.setType(video::CameraType::Free);
	model.camera.setTargetDistance(40.0f);
	model.camera.setRotationType(video::CameraRotationType::Target);
	model.camera.setTarget(glm::vec3(0.0f));
	model.camera.setPosition(glm::vec3(0.0f, 200.0f, 200.0f));

	if (argc == 2) {
		if (!lua_istable(s, 2)) {
			clua_typerror(s, 2, "table");
		}
		lua_pushnil(s);
		while (lua_next(s, 2) != 0) {
			const char *field = luaL_checkstring(s, -2);
			if (!SDL_strcmp(field, "scale")) {
				model.scale = luaL_checknumber(s, -1);
			} else if (!SDL_strcmp(field, "omegaY")) {
				model.omegaY = luaL_checknumber(s, -1);
			} else if (!SDL_strcmp(field, "cameraPos")) {
				const glm::vec3* v = clua_get<glm::vec3>(s, -1);
				model.camera.setPosition(*v);
			} else if (!SDL_strcmp(field, "cameraTarget")) {
				const glm::vec3* v = clua_get<glm::vec3>(s, -1);
				model.camera.setTarget(*v);
			} else {
				clua_assert(s, false, "Unknown field given");
			}
			lua_pop(s, 1);
		}
	}
	nkc_model(cctx, &model);
	return 0;
}

int uilua_edit(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 2);
	nk_flags flags = uilua_checkedittype(s, 1);
	if (!lua_istable(s, 2)) {
		clua_typerror(s, 2, "table");
	}
	lua_getfield(s, 2, "value");
	if (!lua_isstring(s, -1)) {
		luaL_argerror(s, 2, "should have a string value");
	}
	const char *value = lua_tostring(s, -1);
	constexpr size_t UILUA_EDIT_BUFFER_LEN = 1024;
	char editBuffer[UILUA_EDIT_BUFFER_LEN];
	const size_t len = NK_CLAMP(0, (int)SDL_strlen(value), (int)(UILUA_EDIT_BUFFER_LEN - 1));
	core_memcpy(editBuffer, value, len);
	editBuffer[len] = '\0';
	const nk_flags event = nk_edit_string_zero_terminated(ctx, flags, editBuffer, UILUA_EDIT_BUFFER_LEN - 1, nk_filter_default);
	lua_pushstring(s, editBuffer);
	lua_pushvalue(s, -1);
	lua_setfield(s, 2, "value");
	int changed = !lua_equal(s, -1, -2);
	if (event & NK_EDIT_COMMITED) {
		lua_pushstring(s, "commited");
	} else if (event & NK_EDIT_ACTIVATED) {
		lua_pushstring(s, "activated");
	} else if (event & NK_EDIT_DEACTIVATED) {
		lua_pushstring(s, "deactivated");
	} else if (event & NK_EDIT_ACTIVE) {
		lua_pushstring(s, "active");
	} else if (event & NK_EDIT_INACTIVE) {
		lua_pushstring(s, "inactive");
	} else {
		lua_pushnil(s);
	}
	lua_pushboolean(s, changed);
	return 2;
}

int uilua_popup_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) >= 6);
	const nk_popup_type type = uilua_checkpopup(s, 1);
	const char *title = luaL_checkstring(s, 2);
	struct nk_rect bounds;
	bounds.x = luaL_checknumber(s, 3);
	bounds.y = luaL_checknumber(s, 4);
	bounds.w = luaL_checknumber(s, 5);
	bounds.h = luaL_checknumber(s, 6);
	const nk_flags flags = uilua_window_flag(s, 7);
	const int open = nk_popup_begin(ctx, type, title, flags, bounds);
	lua_pushboolean(s, open);
	return 1;
}

int uilua_popup_close(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_popup_close(ctx);
	return 0;
}

int uilua_popup_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_popup_end(ctx);
	return 0;
}

int uilua_combobox(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 2 && argc <= 5);
	if (!lua_istable(s, 2)) {
		clua_typerror(s, 2, "table");
	}
	constexpr int UILUA_COMBOBOX_MAX_ITEMS = 1024;
	const char *combobox_items[UILUA_COMBOBOX_MAX_ITEMS];
	int i;
	for (i = 0; i < UILUA_COMBOBOX_MAX_ITEMS && lua_checkstack(s, 4); ++i) {
		lua_rawgeti(s, 2, i + 1);
		if (lua_isstring(s, -1)) {
			combobox_items[i] = lua_tostring(s, -1);
		} else if (lua_isnil(s, -1)) {
			break;
		} else {
			luaL_argerror(s, 2, "items must be strings");
		}
	}
	struct nk_rect bounds = nk_widget_bounds(ctx);
	int itemHeight = bounds.h;
	if (argc >= 3 && !lua_isnil(s, 3)) {
		itemHeight = luaL_checkinteger(s, 3);
	}
	struct nk_vec2 size = nk_vec2(bounds.w, itemHeight * 8);
	if (argc >= 4 && !lua_isnil(s, 4)) {
		size.x = luaL_checknumber(s, 4);
	}
	if (argc >= 5 && !lua_isnil(s, 5)) {
		size.y = luaL_checknumber(s, 5);
	}
	if (lua_isnumber(s, 1)) {
		int value = lua_tointeger(s, 1) - 1;
		value = nk_combo(ctx, combobox_items, i, value, itemHeight, size);
		lua_pushnumber(s, value + 1);
	} else if (lua_istable(s, 1)) {
		lua_getfield(s, 1, "value");
		if (!lua_isnumber(s, -1)) {
			luaL_argerror(s, 1, "should have a number value");
		}
		int value = lua_tointeger(s, -1) - 1;
		const int old = value;
		nk_combobox(ctx, combobox_items, i, &value, itemHeight, size);
		const int changed = value != old;
		if (changed) {
			lua_pushnumber(s, value + 1);
			lua_setfield(s, 1, "value");
		}
		lua_pushboolean(s, changed);
	} else {
		clua_typerror(s, 1, "number or table");
	}
	return 1;
}

int uilua_combobox_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1 && argc <= 4);
	const char *text = nullptr;
	if (!lua_isnil(s, 1)) {
		text = luaL_checkstring(s, 1);
	}
	struct nk_color color;
	bool useColor = false;
	nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	bool useImage = false;
	if (argc >= 2 && !lua_isnil(s, 2)) {
		if (lua_isstring(s, 2)) {
			if (uilua_is_color(s, 2)) {
				color = uilua_checkcolor(s, 2);
				useColor = true;
			} else {
				symbol = uilua_checksymbol(s, 2);
			}
		} else {
			image = uilua_checkImage(s, 2);
			useImage = true;
		}
	}
	struct nk_rect bounds = nk_widget_bounds(ctx);
	struct nk_vec2 size = nk_vec2(bounds.w, bounds.h * 8);
	if (argc >= 3 && !lua_isnil(s, 3)) {
		size.x = luaL_checknumber(s, 3);
	}
	if (argc >= 4 && !lua_isnil(s, 4)) {
		size.y = luaL_checknumber(s, 4);
	}
	int open = 0;
	if (text != nullptr) {
		if (useColor) {
			clua_assert(s, false, "%s: color comboboxes can't have titles");
		} else if (symbol != NK_SYMBOL_NONE) {
			open = nk_combo_begin_symbol_label(ctx, text, symbol, size);
		} else if (useImage) {
			open = nk_combo_begin_image_label(ctx, text, image, size);
		} else {
			open = nk_combo_begin_label(ctx, text, size);
		}
	} else {
		if (useColor) {
			open = nk_combo_begin_color(ctx, color, size);
		} else if (symbol != NK_SYMBOL_NONE) {
			open = nk_combo_begin_symbol(ctx, symbol, size);
		} else if (useImage) {
			open = nk_combo_begin_image(ctx, image, size);
		} else {
			clua_assert(s, false, "%s: must specify color, symbol, image, and/or title");
		}
	}
	lua_pushboolean(s, open);
	return 1;
}

int uilua_combobox_item(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1 && argc <= 3);
	const char *text = luaL_checkstring(s, 1);
	nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	bool useImage = false;
	if (argc >= 2 && !lua_isnil(s, 2)) {
		if (lua_isstring(s, 2)) {
			symbol = uilua_checksymbol(s, 2);
		} else {
			image = uilua_checkImage(s, 2);
			useImage = true;
		}
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 3 && !lua_isnil(s, 3)) {
		align = uilua_checkalign(s, 3);
	}
	int activated = 0;
	if (symbol != NK_SYMBOL_NONE) {
		activated = nk_combo_item_symbol_label(ctx, symbol, text, align);
	} else if (useImage) {
		activated = nk_combo_item_image_label(ctx, image, text, align);
	} else {
		activated = nk_combo_item_label(ctx, text, align);
	}
	lua_pushboolean(s, activated);
	return 1;
}

int uilua_combobox_close(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_combo_close(ctx);
	return 0;
}

int uilua_combobox_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_combo_end(ctx);
	return 0;
}

int uilua_contextual_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) >= 6);
	struct nk_vec2 size;
	size.x = luaL_checknumber(s, 1);
	size.y = luaL_checknumber(s, 2);
	struct nk_rect trigger;
	trigger.x = luaL_checknumber(s, 3);
	trigger.y = luaL_checknumber(s, 4);
	trigger.w = luaL_checknumber(s, 5);
	trigger.h = luaL_checknumber(s, 6);
	nk_flags flags = uilua_window_flag(s, 7);
	int open = nk_contextual_begin(ctx, flags, size, trigger);
	lua_pushboolean(s, open);
	return 1;
}

int uilua_contextual_item(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 1 && argc <= 3);
	const char *text = luaL_checkstring(s, 1);
	nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	bool useImage = false;
	if (argc >= 2 && !lua_isnil(s, 2)) {
		if (lua_isstring(s, 2)) {
			symbol = uilua_checksymbol(s, 2);
		} else {
			image = uilua_checkImage(s, 2);
			useImage = true;
		}
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 3 && !lua_isnil(s, 3)) {
		align = uilua_checkalign(s, 3);
	}
	int activated;
	if (symbol != NK_SYMBOL_NONE) {
		activated = nk_contextual_item_symbol_label(ctx, symbol, text, align);
	} else if (useImage) {
		activated = nk_contextual_item_image_label(ctx, image, text, align);
	} else {
		activated = nk_contextual_item_label(ctx, text, align);
	}
	lua_pushboolean(s, activated);
	return 1;
}

int uilua_contextual_close(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_contextual_close(ctx);
	return 0;
}

int uilua_contextual_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_contextual_end(ctx);
	return 0;
}

int uilua_tooltip(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const char *text = luaL_checkstring(s, 1);
	nk_tooltip(ctx, text);
	return 0;
}

int uilua_tooltip_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const float width = luaL_checknumber(s, 1);
	const int open = nk_tooltip_begin(ctx, width);
	lua_pushnumber(s, open);
	return 1;
}

int uilua_tooltip_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_tooltip_end(ctx);
	return 0;
}

int uilua_menubar_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_menubar_begin(ctx);
	return 0;
}

int uilua_menubar_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_menubar_end(ctx);
	return 0;
}

int uilua_menu_begin(lua_State *s) {
	const int argc = lua_gettop(s);
	clua_assert_argc(s, argc >= 4 && argc <= 5);
	struct nk_context* ctx = uilua_ctx(s);
	const char *text = luaL_checkstring(s, 1);
	nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	bool useImage = false;
	if (lua_isstring(s, 2)) {
		symbol = uilua_checksymbol(s, 2);
	} else if (!lua_isnil(s, 2)) {
		image = uilua_checkImage(s, 2);
		useImage = true;
	}
	struct nk_vec2 size;
	size.x = luaL_checknumber(s, 3);
	size.y = luaL_checknumber(s, 4);
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 5 && !lua_isnil(s, 5)) {
		align = uilua_checkalign(s, 5);
	}
	int open;
	if (symbol != NK_SYMBOL_NONE) {
		open = nk_menu_begin_symbol_label(ctx, text, align, symbol, size);
	} else if (useImage) {
		open = nk_menu_begin_image_label(ctx, text, align, image, size);
	} else {
		open = nk_menu_begin_label(ctx, text, align, size);
	}
	lua_pushboolean(s, open);
	return 1;
}

int uilua_menu_item(lua_State *s) {
	const int argc = lua_gettop(s);
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, argc >= 1 && argc <= 3);
	const char *text = luaL_checkstring(s, 1);
	nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	bool useImage = false;
	if (argc >= 2 && !lua_isnil(s, 2)) {
		if (lua_isstring(s, 2)) {
			symbol = uilua_checksymbol(s, 2);
		} else {
			image = uilua_checkImage(s, 2);
			useImage = true;
		}
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 3 && !lua_isnil(s, 3)) {
		align = uilua_checkalign(s, 3);
	}
	int activated;
	if (symbol != NK_SYMBOL_NONE) {
		activated = nk_menu_item_symbol_label(ctx, symbol, text, align);
	} else if (useImage) {
		activated = nk_menu_item_image_label(ctx, image, text, align);
	} else {
		activated = nk_menu_item_label(ctx, text, align);
	}
	lua_pushboolean(s, activated);
	return 1;
}

int uilua_menu_close(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_menu_close(ctx);
	return 0;
}

int uilua_menu_end(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	nk_menu_end(ctx);
	return 0;
}

int uilua_spacing(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	const int cols = luaL_checkinteger(s, 1);
	nk_spacing(ctx, cols);
	return 0;
}

int uilua_style_default(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	nk_style_default(ctx);
	return 0;
}

#define UILUA_LOAD_COLOR(type)                                                                                         \
	lua_getfield(s, -1, (type));                                                                                       \
	if (!uilua_is_color(s, -1)) {                                                                                      \
		const char *msg = lua_pushfstring(s, "%%s: table missing color value for '%s'", type);                         \
		clua_assert(s, 0, msg);                                                                                        \
	}                                                                                                                  \
	colors[index++] = uilua_checkcolor(s, -1);                                                                         \
	lua_pop(s, 1)

int uilua_style_load_colors(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	if (!lua_istable(s, 1))
		clua_typerror(s, 1, "table");
	struct nk_color colors[NK_COLOR_COUNT];
	int index = 0;
	UILUA_LOAD_COLOR("text");
	UILUA_LOAD_COLOR("window");
	UILUA_LOAD_COLOR("header");
	UILUA_LOAD_COLOR("border");
	UILUA_LOAD_COLOR("button");
	UILUA_LOAD_COLOR("button hover");
	UILUA_LOAD_COLOR("button active");
	UILUA_LOAD_COLOR("toggle");
	UILUA_LOAD_COLOR("toggle hover");
	UILUA_LOAD_COLOR("toggle cursor");
	UILUA_LOAD_COLOR("select");
	UILUA_LOAD_COLOR("select active");
	UILUA_LOAD_COLOR("slider");
	UILUA_LOAD_COLOR("slider cursor");
	UILUA_LOAD_COLOR("slider cursor hover");
	UILUA_LOAD_COLOR("slider cursor active");
	UILUA_LOAD_COLOR("property");
	UILUA_LOAD_COLOR("edit");
	UILUA_LOAD_COLOR("edit cursor");
	UILUA_LOAD_COLOR("combo");
	UILUA_LOAD_COLOR("chart");
	UILUA_LOAD_COLOR("chart color");
	UILUA_LOAD_COLOR("chart color highlight");
	UILUA_LOAD_COLOR("scrollbar");
	UILUA_LOAD_COLOR("scrollbar cursor");
	UILUA_LOAD_COLOR("scrollbar cursor hover");
	UILUA_LOAD_COLOR("scrollbar cursor active");
	UILUA_LOAD_COLOR("tab header");
	core_assert(index == NK_COLOR_COUNT);
	nk_style_from_table(ctx, colors);
	return 0;
}

int uilua_style_set_font(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	LUAUIApp* app = uilua_app(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	nk_style_set_font(ctx, &app->font(luaL_checkinteger(s, 1))->handle);
	return 0;
}

static int uilua_style_push_color(lua_State* s, struct nk_color *field) {
	struct nk_context* ctx = uilua_ctx(s);
	if (!uilua_is_color(s, -1)) {
		const char *msg = lua_pushfstring(s, "%%s: bad color string '%s'", lua_tostring(s, -1));
		clua_assert(s, 0, msg);
	}
	struct nk_color color = uilua_checkcolor(s, -1);
	int success = nk_style_push_color(ctx, field, color);
	if (success) {
		lua_pushstring(s, "color");
		size_t stack_size = lua_objlen(s, 1);
		lua_rawseti(s, 1, stack_size + 1);
	}
	return 0;
}

static int uilua_style_push_vec2(lua_State* s, struct nk_vec2 *field) {
	struct nk_context* ctx = uilua_ctx(s);
	static const char *msg = "%s: vec2 fields must have x and y components";
	clua_assert(s, lua_istable(s, -1), msg);
	lua_getfield(s, -1, "x");
	clua_assert(s, lua_isnumber(s, -1), msg);
	lua_getfield(s, -2, "y");
	clua_assert(s, lua_isnumber(s, -1), msg);
	struct nk_vec2 vec2;
	vec2.x = lua_tonumber(s, -2);
	vec2.y = lua_tonumber(s, -1);
	lua_pop(s, 2);
	int success = nk_style_push_vec2(ctx, field, vec2);
	if (success) {
		lua_pushstring(s, "vec2");
		size_t stack_size = lua_objlen(s, 1);
		lua_rawseti(s, 1, stack_size + 1);
	}
	return success;
}

static int uilua_style_push_item(lua_State* s, struct nk_style_item *field) {
	struct nk_context* ctx = uilua_ctx(s);
	struct nk_style_item item;
	if (uilua_is_color(s, -1)) {
		item.type = NK_STYLE_ITEM_COLOR;
		item.data.color = uilua_checkcolor(s, -1);
	} else if (lua_isstring(s, -1)) {
		item.type = NK_STYLE_ITEM_IMAGE;
		item.data.image = uilua_checkImage(s, -1);
	} else {
		clua_assert(s, false, "Expected to get color or string (for image)");
	}
	int success = nk_style_push_style_item(ctx, field, item);
	if (success) {
		lua_pushstring(s, "item");
		size_t stack_size = lua_objlen(s, 1);
		lua_rawseti(s, 1, stack_size + 1);
	}
	return success;
}

static int uilua_style_push_align(lua_State* s, nk_flags *field) {
	struct nk_context* ctx = uilua_ctx(s);
	nk_flags align = uilua_checkalign(s, -1);
	int success = nk_style_push_flags(ctx, field, align);
	if (success) {
		lua_pushstring(s, "flags");
		size_t stack_size = lua_objlen(s, 1);
		lua_rawseti(s, 1, stack_size + 1);
	}
	return success;
}

static int uilua_style_push_float(lua_State* s, float *field) {
	struct nk_context* ctx = uilua_ctx(s);
	float f = luaL_checknumber(s, -1);
	int success = nk_style_push_float(ctx, field, f);
	if (success) {
		lua_pushstring(s, "float");
		size_t stack_size = lua_objlen(s, 1);
		lua_rawseti(s, 1, stack_size + 1);
	}
	return success;
}

static int uilua_style_push_font(lua_State* s, const struct nk_user_font **field) {
	struct nk_context* ctx = uilua_ctx(s);
	LUAUIApp* app = uilua_app(s);
	clua_assert(s, lua_isinteger(s, -1), "%s: font field must be an integer for the font size");
	const int fontSize = luaL_checkinteger(s, -1);
	int success = nk_style_push_font(ctx, &app->font(fontSize)->handle);
	if (success) {
		lua_pushstring(s, "font");
		size_t stack_size = lua_objlen(s, 1);
		lua_rawseti(s, 1, stack_size + 1);
	}
	return success;
}

#define UILUA_STYLE_PUSH(name, type, field)                                                                            \
	clua_assert(s, lua_istable(s, -1), "%s: " name " field must be a table");                                          \
	lua_getfield(s, -1, name);                                                                                         \
	if (!lua_isnil(s, -1))                                                                                             \
		uilua_style_push_##type(s, field);                                                                             \
	lua_pop(s, 1);

static void uilua_style_push_text(lua_State* s, struct nk_style_text *style) {
	clua_assert(s, lua_istable(s, -1), "%s: text style must be a table");
	UILUA_STYLE_PUSH("color", color, &style->color);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
}

static void uilua_style_push_button(lua_State* s, struct nk_style_button *style) {
	clua_assert(s, lua_istable(s, -1), "%s: button style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("text background", color, &style->text_background);
	UILUA_STYLE_PUSH("text normal", color, &style->text_normal);
	UILUA_STYLE_PUSH("text hover", color, &style->text_hover);
	UILUA_STYLE_PUSH("text active", color, &style->text_active);
	UILUA_STYLE_PUSH("text alignment", align, &style->text_alignment);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("image padding", vec2, &style->image_padding);
	UILUA_STYLE_PUSH("touch padding", vec2, &style->touch_padding);
}

static void uilua_style_push_scrollbar(lua_State* s, struct nk_style_scrollbar *style) {
	clua_assert(s, lua_istable(s, -1), "%s: scrollbar style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	UILUA_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	UILUA_STYLE_PUSH("cursor active", item, &style->active);
	UILUA_STYLE_PUSH("cursor border color", color, &style->cursor_border_color);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("border cursor", float, &style->border_cursor);
	UILUA_STYLE_PUSH("rounding cursor", float, &style->rounding_cursor);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
}

static void uilua_style_push_edit(lua_State* s, struct nk_style_edit *style) {
	clua_assert(s, lua_istable(s, -1), "%s: edit style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("scrollbar", scrollbar, &style->scrollbar);
	UILUA_STYLE_PUSH("cursor normal", color, &style->cursor_normal);
	UILUA_STYLE_PUSH("cursor hover", color, &style->cursor_hover);
	UILUA_STYLE_PUSH("cursor text normal", color, &style->cursor_text_normal);
	UILUA_STYLE_PUSH("cursor text hover", color, &style->cursor_text_hover);
	UILUA_STYLE_PUSH("text normal", color, &style->text_normal);
	UILUA_STYLE_PUSH("text hover", color, &style->text_hover);
	UILUA_STYLE_PUSH("text active", color, &style->text_active);
	UILUA_STYLE_PUSH("selected normal", color, &style->selected_normal);
	UILUA_STYLE_PUSH("selected hover", color, &style->selected_hover);
	UILUA_STYLE_PUSH("selected text normal", color, &style->text_normal);
	UILUA_STYLE_PUSH("selected text hover", color, &style->selected_text_hover);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("cursor size", float, &style->cursor_size);
	UILUA_STYLE_PUSH("scrollbar size", vec2, &style->scrollbar_size);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("row padding", float, &style->row_padding);
}

static void uilua_style_push_toggle(lua_State* s, struct nk_style_toggle *style) {
	clua_assert(s, lua_istable(s, -1), "%s: toggle style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	UILUA_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	UILUA_STYLE_PUSH("text normal", color, &style->text_normal);
	UILUA_STYLE_PUSH("text hover", color, &style->text_hover);
	UILUA_STYLE_PUSH("text active", color, &style->text_active);
	UILUA_STYLE_PUSH("text background", color, &style->text_background);
	UILUA_STYLE_PUSH("text alignment", align, &style->text_alignment);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("touch padding", vec2, &style->touch_padding);
	UILUA_STYLE_PUSH("spacing", float, &style->spacing);
	UILUA_STYLE_PUSH("border", float, &style->border);
}

static void uilua_style_push_selectable(lua_State* s, struct nk_style_selectable *style) {
	clua_assert(s, lua_istable(s, -1), "%s: selectable style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("pressed", item, &style->pressed);
	UILUA_STYLE_PUSH("normal active", item, &style->normal_active);
	UILUA_STYLE_PUSH("hover active", item, &style->hover_active);
	UILUA_STYLE_PUSH("pressed active", item, &style->pressed_active);
	UILUA_STYLE_PUSH("text normal", color, &style->text_normal);
	UILUA_STYLE_PUSH("text hover", color, &style->text_hover);
	UILUA_STYLE_PUSH("text pressed", color, &style->text_pressed);
	UILUA_STYLE_PUSH("text normal active", color, &style->text_normal_active);
	UILUA_STYLE_PUSH("text hover active", color, &style->text_hover_active);
	UILUA_STYLE_PUSH("text pressed active", color, &style->text_pressed_active);
	UILUA_STYLE_PUSH("text background", color, &style->text_background);
	UILUA_STYLE_PUSH("text alignment", align, &style->text_alignment);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("touch padding", vec2, &style->touch_padding);
	UILUA_STYLE_PUSH("image padding", vec2, &style->image_padding);
}

static void uilua_style_push_slider(lua_State* s, struct nk_style_slider *style) {
	clua_assert(s, lua_istable(s, -1), "%s: slider style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("bar normal", color, &style->bar_normal);
	UILUA_STYLE_PUSH("bar active", color, &style->bar_active);
	UILUA_STYLE_PUSH("bar filled", color, &style->bar_filled);
	UILUA_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	UILUA_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	UILUA_STYLE_PUSH("cursor active", item, &style->cursor_active);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("bar height", float, &style->bar_height);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("spacing", vec2, &style->spacing);
	UILUA_STYLE_PUSH("cursor size", vec2, &style->cursor_size);
}

static void uilua_style_push_progress(lua_State* s, struct nk_style_progress *style) {
	clua_assert(s, lua_istable(s, -1), "%s: progress style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	UILUA_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	UILUA_STYLE_PUSH("cusor active", item, &style->cursor_active);
	UILUA_STYLE_PUSH("cursor border color", color, &style->cursor_border_color);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("cursor border", float, &style->cursor_border);
	UILUA_STYLE_PUSH("cursor rounding", float, &style->cursor_rounding);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
}

static void uilua_style_push_property(lua_State* s, struct nk_style_property *style) {
	clua_assert(s, lua_istable(s, -1), "%s: property style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("label normal", color, &style->label_normal);
	UILUA_STYLE_PUSH("label hover", color, &style->label_hover);
	UILUA_STYLE_PUSH("label active", color, &style->label_active);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("edit", edit, &style->edit);
	UILUA_STYLE_PUSH("inc button", button, &style->inc_button);
	UILUA_STYLE_PUSH("dec button", button, &style->dec_button);
}

static void uilua_style_push_chart(lua_State* s, struct nk_style_chart *style) {
	clua_assert(s, lua_istable(s, -1), "%s: chart style must be a table");
	UILUA_STYLE_PUSH("background", item, &style->background);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("selected color", color, &style->selected_color);
	UILUA_STYLE_PUSH("color", color, &style->color);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
}

static void uilua_style_push_tab(lua_State* s, struct nk_style_tab *style) {
	clua_assert(s, lua_istable(s, -1), "%s: tab style must be a table");
	UILUA_STYLE_PUSH("background", item, &style->background);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("text", color, &style->text);
	UILUA_STYLE_PUSH("tab maximize button", button, &style->tab_maximize_button);
	UILUA_STYLE_PUSH("tab minimize button", button, &style->tab_minimize_button);
	UILUA_STYLE_PUSH("node maximize button", button, &style->node_maximize_button);
	UILUA_STYLE_PUSH("node minimize button", button, &style->node_minimize_button);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("indent", float, &style->indent);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("spacing", vec2, &style->spacing);
}

static void uilua_style_push_combo(lua_State* s, struct nk_style_combo *style) {
	clua_assert(s, lua_istable(s, -1), "%s: combo style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("label normal", color, &style->label_normal);
	UILUA_STYLE_PUSH("label hover", color, &style->label_hover);
	UILUA_STYLE_PUSH("label active", color, &style->label_active);
	UILUA_STYLE_PUSH("symbol normal", color, &style->symbol_normal);
	UILUA_STYLE_PUSH("symbol hover", color, &style->symbol_hover);
	UILUA_STYLE_PUSH("symbol active", color, &style->symbol_active);
	UILUA_STYLE_PUSH("button", button, &style->button);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("content padding", vec2, &style->content_padding);
	UILUA_STYLE_PUSH("button padding", vec2, &style->button_padding);
	UILUA_STYLE_PUSH("spacing", vec2, &style->spacing);
}

static void uilua_style_push_window_header(lua_State* s, struct nk_style_window_header *style) {
	clua_assert(s, lua_istable(s, -1), "%s: window header style must be a table");
	UILUA_STYLE_PUSH("normal", item, &style->normal);
	UILUA_STYLE_PUSH("hover", item, &style->hover);
	UILUA_STYLE_PUSH("active", item, &style->active);
	UILUA_STYLE_PUSH("close button", button, &style->close_button);
	UILUA_STYLE_PUSH("minimize button", button, &style->minimize_button);
	UILUA_STYLE_PUSH("label normal", color, &style->label_normal);
	UILUA_STYLE_PUSH("label hover", color, &style->label_hover);
	UILUA_STYLE_PUSH("label active", color, &style->label_active);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("label padding", vec2, &style->label_padding);
	UILUA_STYLE_PUSH("spacing", vec2, &style->spacing);
}

static void uilua_style_push_window(lua_State* s, struct nk_style_window *style) {
	clua_assert(s, lua_istable(s, -1), "%s: window style must be a table");
	UILUA_STYLE_PUSH("header", window_header, &style->header);
	UILUA_STYLE_PUSH("fixed background", item, &style->fixed_background);
	UILUA_STYLE_PUSH("background", color, &style->background);
	UILUA_STYLE_PUSH("border color", color, &style->border_color);
	UILUA_STYLE_PUSH("popup border color", color, &style->popup_border_color);
	UILUA_STYLE_PUSH("combo border color", color, &style->combo_border_color);
	UILUA_STYLE_PUSH("contextual border color", color, &style->contextual_border_color);
	UILUA_STYLE_PUSH("menu border color", color, &style->menu_border_color);
	UILUA_STYLE_PUSH("group border color", color, &style->group_border_color);
	UILUA_STYLE_PUSH("tooltip border color", color, &style->tooltip_border_color);
	UILUA_STYLE_PUSH("scaler", item, &style->scaler);
	UILUA_STYLE_PUSH("border", float, &style->border);
	UILUA_STYLE_PUSH("combo border", float, &style->combo_border);
	UILUA_STYLE_PUSH("contextual border", float, &style->contextual_border);
	UILUA_STYLE_PUSH("menu border", float, &style->menu_border);
	UILUA_STYLE_PUSH("group border", float, &style->group_border);
	UILUA_STYLE_PUSH("tooltip border", float, &style->tooltip_border);
	UILUA_STYLE_PUSH("popup border", float, &style->popup_border);
	UILUA_STYLE_PUSH("rounding", float, &style->rounding);
	UILUA_STYLE_PUSH("spacing", vec2, &style->spacing);
	UILUA_STYLE_PUSH("scrollbar size", vec2, &style->scrollbar_size);
	UILUA_STYLE_PUSH("min size", vec2, &style->min_size);
	UILUA_STYLE_PUSH("padding", vec2, &style->padding);
	UILUA_STYLE_PUSH("group padding", vec2, &style->group_padding);
	UILUA_STYLE_PUSH("popup padding", vec2, &style->popup_padding);
	UILUA_STYLE_PUSH("combo padding", vec2, &style->combo_padding);
	UILUA_STYLE_PUSH("contextual padding", vec2, &style->contextual_padding);
	UILUA_STYLE_PUSH("menu padding", vec2, &style->menu_padding);
	UILUA_STYLE_PUSH("tooltip padding", vec2, &style->tooltip_padding);
}

int uilua_style_push(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 1);
	if (!lua_istable(s, 1))
		clua_typerror(s, 1, "table");
	lua_newtable(s);
	lua_insert(s, 1);
	UILUA_STYLE_PUSH("font", font, &ctx->style.font);
	UILUA_STYLE_PUSH("text", text, &ctx->style.text);
	UILUA_STYLE_PUSH("button", button, &ctx->style.button);
	UILUA_STYLE_PUSH("contextual button", button, &ctx->style.contextual_button);
	UILUA_STYLE_PUSH("menu button", button, &ctx->style.menu_button);
	UILUA_STYLE_PUSH("option", toggle, &ctx->style.option);
	UILUA_STYLE_PUSH("checkbox", toggle, &ctx->style.checkbox);
	UILUA_STYLE_PUSH("selectable", selectable, &ctx->style.selectable);
	UILUA_STYLE_PUSH("slider", slider, &ctx->style.slider);
	UILUA_STYLE_PUSH("progress", progress, &ctx->style.progress);
	UILUA_STYLE_PUSH("property", property, &ctx->style.property);
	UILUA_STYLE_PUSH("edit", edit, &ctx->style.edit);
	UILUA_STYLE_PUSH("chart", chart, &ctx->style.chart);
	UILUA_STYLE_PUSH("scrollh", scrollbar, &ctx->style.scrollh);
	UILUA_STYLE_PUSH("scrollv", scrollbar, &ctx->style.scrollv);
	UILUA_STYLE_PUSH("tab", tab, &ctx->style.tab);
	UILUA_STYLE_PUSH("combo", combo, &ctx->style.combo);
	UILUA_STYLE_PUSH("window", window, &ctx->style.window);
	lua_pop(s, 1);
	lua_getglobal(s, "stack");
	size_t stack_size = lua_objlen(s, -1);
	lua_pushvalue(s, 1);
	lua_rawseti(s, -2, stack_size + 1);
	return 0;
}

int uilua_style_pop(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	lua_getglobal(s, "stack");
	clua_assert(s, lua_istable(s, -1), "Global 'stack' not found");
	size_t stack_size = lua_objlen(s, -1);
	lua_rawgeti(s, -1, stack_size);
	lua_pushnil(s);
	lua_rawseti(s, -3, stack_size);
	stack_size = lua_objlen(s, -1);
	size_t i;
	for (i = stack_size; i > 0; --i) {
		lua_rawgeti(s, -1, i);
		const char *type = lua_tostring(s, -1);
		if (!SDL_strcmp(type, "color")) {
			nk_style_pop_color(ctx);
		} else if (!SDL_strcmp(type, "vec2")) {
			nk_style_pop_vec2(ctx);
		} else if (!SDL_strcmp(type, "item")) {
			nk_style_pop_style_item(ctx);
		} else if (!SDL_strcmp(type, "flags")) {
			nk_style_pop_flags(ctx);
		} else if (!SDL_strcmp(type, "float")) {
			nk_style_pop_float(ctx);
		} else if (!SDL_strcmp(type, "font")) {
			nk_style_pop_font(ctx);
		} else {
			const char *msg = lua_pushfstring(s, "%%s: bad style item type '%s'", type);
			clua_assert(s, 0, msg);
		}
		lua_pop(s, 1);
	}
	return 0;
}

int uilua_style(lua_State *s) {
	clua_assert(s, lua_checkstack(s, 3), "%s: failed to allocate stack space");
	clua_assert_argc(s, lua_gettop(s) == 3);
	if (!lua_isfunction(s, -1))
		clua_typerror(s, lua_gettop(s), "function");
	lua_pushvalue(s, 1);
	lua_insert(s, 2);
	lua_pushvalue(s, 1);
	lua_insert(s, 3);
	lua_insert(s, 2);
	lua_getfield(s, 1, "stylePush");
	lua_insert(s, 4);
	lua_call(s, 2, 0);
	lua_call(s, 1, 0);
	lua_getfield(s, 1, "stylePop");
	lua_insert(s, 1);
	lua_call(s, 1, 0);
	return 0;
}

int uilua_widget_bounds(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_rect bounds = nk_widget_bounds(ctx);
	lua_pushnumber(s, bounds.x);
	lua_pushnumber(s, bounds.y);
	lua_pushnumber(s, bounds.w);
	lua_pushnumber(s, bounds.h);
	return 4;
}

int uilua_widget_position(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_vec2 pos = nk_widget_position(ctx);
	lua_pushnumber(s, pos.x);
	lua_pushnumber(s, pos.y);
	return 2;
}

int uilua_widget_size(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	struct nk_vec2 pos = nk_widget_size(ctx);
	lua_pushnumber(s, pos.x);
	lua_pushnumber(s, pos.y);
	return 2;
}

int uilua_widget_width(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	float width = nk_widget_width(ctx);
	lua_pushnumber(s, width);
	return 1;
}

int uilua_widget_height(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	float height = nk_widget_height(ctx);
	lua_pushnumber(s, height);
	return 1;
}

int uilua_widget_is_hovered(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	clua_assert_argc(s, lua_gettop(s) == 0);
	int hovered = nk_widget_is_hovered(ctx);
	lua_pushboolean(s, hovered);
	return 1;
}

int uilua_global_alpha(lua_State *s) {
	const float globalAlpha = luaL_checknumber(s, 1);
	uilua_app(s)->setGlobalAlpha(globalAlpha);
	return 0;
}

int uilua_window_root(lua_State *s) {
	const char* id = luaL_checkstring(s, 1);
	uilua_app(s)->rootWindow(id);
	return 0;
}

int uilua_window_push(lua_State *s) {
	const char* id = luaL_checkstring(s, 1);
	const char* parameter = luaL_optstring(s, 2, "");
	uilua_app(s)->pushWindow(id, parameter);
	return 0;
}

int uilua_window_pop(lua_State *s) {
	uilua_app(s)->popWindow();
	return 0;
}

}
}
