/**
 * @file
 * @brief LUA bindings for nuklear ui.
 */

#pragma once

#include "commonlua/LUAFunctions.h"
#include "Nuklear.h"
#include "core/Assert.h"
#include <vector>

namespace nuklear {

static inline struct nk_context* uilua_ctx(lua_State* s) {
	return lua::LUA::globalData<struct nk_context>(s, "context");
}

static constexpr struct Flags {
	const char *name;
	nk_flags flag;
} windowFlags[] = {
	{ "scrollbar", (nk_flags)~NK_WINDOW_NO_SCROLLBAR },
	{ "scroll auto hide", NK_WINDOW_SCROLL_AUTO_HIDE },
	{ "minimizable", NK_WINDOW_MINIMIZABLE },
	{ "background", NK_WINDOW_BACKGROUND },
	{ "scalable", NK_WINDOW_SCALABLE },
	{ "closable", NK_WINDOW_CLOSABLE },
	{ "movable", NK_WINDOW_MOVABLE },
	{ "border", NK_WINDOW_BORDER },
	{ "title", NK_WINDOW_TITLE },
	{ nullptr, 0u }
};

/**
 * @brief Convert the given window flags to nuklear enum values
 */
static nk_flags uilua_window_flag(lua_State *s, int argsStartIndex) {
	const int argc = lua_gettop(s);
	nk_flags flags = NK_WINDOW_NO_SCROLLBAR;
	for (int i = argsStartIndex; i <= argc; ++i) {
		const char *flagId = luaL_checkstring(s, i);
		for (const Flags* flag = windowFlags; flag->name != nullptr; ++flag) {
			if (!strcmp(flag->name, flagId)) {
				flags |= flag->flag;
				break;
			}
		}
		const char *msg = lua_pushfstring(s, "Unknown window flag given: '%s'", flagId);
		return luaL_argerror(s, i, msg);
	}
	return flags;
}

/**
 * @brief Extended window start with separated title and identifier to allow multiple windows with same name but not title
 * @param[in] name (optional) The name of the window - if not given, the name will be the title. Needs to be persistent over
 * frames to identify the window
 * @param[in] title The title of the window displayed inside header if flag @c title or either @c closable or @c minimized was set
 * @param[in] x Position of the window
 * @param[in] y Position of the window
 * @param[in] w Size of the window
 * @param[in] h Size of the window
 * @param[in] flags Window flags (scrollbar, scroll auto hide, minimizable, background, scalable, closable, movable, border, title)
 * with a number of different window behaviors
 * @note If you do not define @c scalable or @c moveable you can set window position and size every frame
 * @return @c true if the window can be filled up with widgets from this point until @c windowEnd or @c false otherwise
 */
int uilua_window_begin(lua_State *s) {
	struct nk_context* ctx = uilua_ctx(s);
	const char *name;
	const char *title;
	int startIndex;
	const bool noNameDefined = lua_isnumber(s, 2);
	if (noNameDefined) {
		core_assert(lua_gettop(s) >= 5);
		name = title = luaL_checkstring(s, 1);
		startIndex = 2;
	} else {
		core_assert(lua_gettop(s) >= 6);
		name = luaL_checkstring(s, 1);
		title = luaL_checkstring(s, 2);
		startIndex = 3;
	}
	const float x = luaL_checknumber(s, startIndex++);
	const float y = luaL_checknumber(s, startIndex++);
	const float w = luaL_checknumber(s, startIndex++);
	const float h = luaL_checknumber(s, startIndex++);
	const nk_flags flags = uilua_window_flag(s, startIndex);
	const int retVal = nk_begin_titled(ctx, name, title, nk_rect(x, y, w, h), flags);
	lua_pushboolean(s, retVal);
	return 1;
}

/**
 * @brief Needs to be called at the end of the window building process to process scaling,
 * scrollbars and general cleanup. All widget calls after this functions will result in
 * asserts or no state changes
 */
int uilua_window_end(lua_State *s) {
	core_assert(lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	nk_end(ctx);
	return 0;
}

/**
 * @return A rectangle with screen position and size of the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
int uilua_window_get_bounds(lua_State *s) {
	core_assert(lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_rect& rect = nk_window_get_bounds(ctx);
	lua_pushnumber(s, rect.x);
	lua_pushnumber(s, rect.y);
	lua_pushnumber(s, rect.w);
	lua_pushnumber(s, rect.h);
	return 4;
}

/**
 * @return The position of the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
int uilua_window_get_position(lua_State *s) {
	core_assert(lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_vec2& pos = nk_window_get_position(ctx);
	lua_pushnumber(s, pos.x);
	lua_pushnumber(s, pos.y);
	return 2;
}

/**
 * @return The size with width and height of the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
int uilua_window_get_size(lua_State *s) {
	core_assert(lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_vec2& size = nk_window_get_size(ctx);
	lua_pushnumber(s, size.x);
	lua_pushnumber(s, size.y);
	return 2;
}

/**
 * @return The position and size of the currently visible and non-clipped space inside the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
int uilua_window_get_content_region(lua_State *s) {
	core_assert(lua_gettop(s) == 0);
	struct nk_context* ctx = uilua_ctx(s);
	const struct nk_rect& rect = nk_window_get_content_region(ctx);
	lua_pushnumber(s, rect.x);
	lua_pushnumber(s, rect.y);
	lua_pushnumber(s, rect.w);
	lua_pushnumber(s, rect.h);
	return 4;
}

}
