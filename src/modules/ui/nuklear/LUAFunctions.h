/**
 * @file
 * @brief LUA bindings for nuklear ui.
 * @note Most of this stuff is shamelessly copied over from nuklear-love lua bindings
 * @ingroup UI
 *
 * LOVE-Nuklear - MIT licensed; no warranty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 * adapted to LOVE in 2016 by Kevin Harrison
 */

#pragma once

#include "commonlua/LUAFunctions.h"
#include "Nuklear.h"
#include "core/Assert.h"
#include <vector>

namespace ui {
namespace nuklear {

/**
 * @brief Extended window start with separated title and identifier to allow multiple windows with same name but not title
 * @par name (optional) The name of the window - if not given, the name will be the title. Needs to be persistent over
 * frames to identify the window
 * @par title The title of the window displayed inside header if flag @c title or either @c closable or @c minimized was set
 * @par x Position of the window
 * @par y Position of the window
 * @par w Size of the window
 * @par h Size of the window
 * @par flags Window flags (scrollbar, scroll auto hide, minimizable, background, scalable, closable, movable, border, title)
 * with a number of different window behaviors
 * @note If you do not define @c scalable or @c moveable you can set window position and size every frame
 * @return @c true if the window can be filled up with widgets from this point until @c windowEnd or @c false otherwise
 */
extern int uilua_window_begin(lua_State *s);

/**
 * @brief Needs to be called at the end of the window building process to process scaling,
 * scrollbars and general cleanup. All widget calls after this functions will result in
 * asserts or no state changes
 */
extern int uilua_window_end(lua_State *s);

/**
 * @return A rectangle with screen position and size of the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
extern int uilua_window_get_bounds(lua_State *s);

/**
 * @return The position of the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
extern int uilua_window_get_position(lua_State *s);

/**
 * @return The size with width and height of the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
extern int uilua_window_get_size(lua_State *s);

/**
 * @return The position and size of the currently visible and non-clipped space inside the currently processed window.
 * @note IMPORTANT: only call this function between calls `windowBegin` and `windowEnd`
 */
extern int uilua_window_get_content_region(lua_State *s);

extern int uilua_model(lua_State *s);
extern int uilua_edit(lua_State *s);
extern int uilua_text(lua_State *s);
extern int uilua_push_scissor(lua_State *s);
extern int uilua_label(lua_State *s);
extern int uilua_image(lua_State *s);
extern int uilua_button(lua_State *s);

extern int uilua_window_has_focus(lua_State *s);
extern int uilua_window_is_collapsed(lua_State *s);
extern int uilua_window_is_hidden(lua_State *s);
extern int uilua_window_is_active(lua_State *s);
extern int uilua_window_is_hovered(lua_State *s);
extern int uilua_window_is_any_hovered(lua_State *s);
extern int uilua_item_is_any_active(lua_State *s);
extern int uilua_window_set_bounds(lua_State *s);
extern int uilua_window_set_position(lua_State *s);
extern int uilua_window_set_size(lua_State *s);
extern int uilua_window_set_focus(lua_State *s);
extern int uilua_window_close(lua_State *s);
extern int uilua_window_collapse(lua_State *s);
extern int uilua_window_expand(lua_State *s);
extern int uilua_window_show(lua_State *s);
extern int uilua_window_hide(lua_State *s);
extern int uilua_layout_row(lua_State *s);
extern int uilua_layout_row_begin(lua_State *s);
extern int uilua_layout_row_push(lua_State *s);
extern int uilua_layout_row_end(lua_State *s);
extern int uilua_layout_space_begin(lua_State *s);
extern int uilua_layout_space_push(lua_State *s);
extern int uilua_layout_space_end(lua_State *s);
extern int uilua_layout_space_bounds(lua_State *s);
extern int uilua_layout_space_to_screen(lua_State *s);
extern int uilua_layout_space_to_local(lua_State *s);
extern int uilua_layout_space_rect_to_screen(lua_State *s);
extern int uilua_layout_space_rect_to_local(lua_State *s);
extern int uilua_layout_ratio_from_pixel(lua_State *s);
extern int uilua_group_begin(lua_State *s);
extern int uilua_group_end(lua_State *s);
extern int uilua_tree_push(lua_State *s);
extern int uilua_tree_pop(lua_State *s);
extern int uilua_button_set_behavior(lua_State *s);
extern int uilua_button_push_behavior(lua_State *s);
extern int uilua_button_pop_behavior(lua_State *s);
extern int uilua_checkbox(lua_State *s);
extern int uilua_radio(lua_State *s);
extern int uilua_selectable(lua_State *s);
extern int uilua_slider(lua_State *s);
extern int uilua_progress(lua_State *s);
extern int uilua_color_picker(lua_State *s);
extern int uilua_property(lua_State *s);
extern int uilua_popup_begin(lua_State *s);
extern int uilua_popup_close(lua_State *s);
extern int uilua_popup_end(lua_State *s);
extern int uilua_combobox(lua_State *s);
extern int uilua_combobox_begin(lua_State *s);
extern int uilua_combobox_item(lua_State *s);
extern int uilua_combobox_close(lua_State *s);
extern int uilua_combobox_end(lua_State *s);
extern int uilua_contextual_begin(lua_State *s);
extern int uilua_contextual_item(lua_State *s);
extern int uilua_contextual_close(lua_State *s);
extern int uilua_contextual_end(lua_State *s);
extern int uilua_tooltip(lua_State *s);
extern int uilua_tooltip_begin(lua_State *s);
extern int uilua_tooltip_end(lua_State *s);
extern int uilua_menubar_begin(lua_State *s);
extern int uilua_menubar_end(lua_State *s);
extern int uilua_menu_begin(lua_State *s);
extern int uilua_menu_item(lua_State *s);
extern int uilua_menu_close(lua_State *s);
extern int uilua_menu_end(lua_State *s);
extern int uilua_spacing(lua_State *s);
extern int uilua_style_default(lua_State *s);
extern int uilua_style_load_colors(lua_State *s);
extern int uilua_style_set_font(lua_State *s);
extern int uilua_style_push(lua_State *s);
extern int uilua_style_pop(lua_State *s);
extern int uilua_style(lua_State *s);
extern int uilua_widget_bounds(lua_State *s);
extern int uilua_widget_position(lua_State *s);
extern int uilua_widget_size(lua_State *s);
extern int uilua_widget_width(lua_State *s);
extern int uilua_widget_height(lua_State *s);
extern int uilua_widget_is_hovered(lua_State *s);

extern int uilua_window_push(lua_State *s);
extern int uilua_window_pop(lua_State *s);
extern int uilua_window_root(lua_State *s);
extern int uilua_global_alpha(lua_State *s);

}
}
