set(LIB turbobadger)
set(SRCS
	tb/animation/tb_animation_utils.h
	tb/animation/tb_animation.cpp
	tb/animation/tb_widget_animation.h
	tb/animation/tb_animation.h
	tb/animation/tb_widget_animation.cpp
	tb/utf8/utf8.cpp
	tb/utf8/utf8.h
	tb/renderers/tb_renderer_batcher.cpp
	tb/renderers/tb_renderer_batcher.h
	tb/thirdparty/stb_truetype.h
	tb/parser/tb_parser.h
	tb/parser/tb_parser.cpp
	tb/image/tb_image_manager.cpp
	tb/image/tb_image_widget.cpp
	tb/image/tb_image_widget.h
	tb/image/tb_image_manager.h
	tb/tb_bitmap_fragment.cpp
	tb/tb_color.cpp
	tb/tb_core.cpp
	tb/tb_debug.cpp
	tb/tb_dimension.cpp
	tb/tb_editfield.cpp
	tb/tb_font_renderer.cpp
	tb/tb_font_renderer_stb.cpp
	tb/tb_font_renderer_tbbf.cpp
	tb/tb_geometry.cpp
	tb/tb_hashtable.cpp
	tb/tb_inline_select.cpp
	tb/tb_language.cpp
	tb/tb_layout.cpp
	tb/tb_linklist.cpp
	tb/tb_list.cpp
	tb/tb_menu_window.cpp
	tb/tb_message_window.cpp
	tb/tb_msg.cpp
	tb/tb_node_ref_tree.cpp
	tb/tb_node_tree.cpp
	tb/tb_popup_window.cpp
	tb/tb_renderer.cpp
	tb/tb_scroll_container.cpp
	tb/tb_scroller.cpp
	tb/tb_select.cpp
	tb/tb_select_item.cpp
	tb/tb_skin.cpp
	tb/tb_skin_util.cpp
	tb/tb_str.cpp
	tb/tb_style_edit.cpp
	tb/tb_style_edit_content.cpp
	tb/tb_system.cpp
	tb/tb_tab_container.cpp
	tb/tb_tempbuffer.cpp
	tb/tb_toggle_container.cpp
	tb/tb_value.cpp
	tb/tb_widget_skin_condition_context.cpp
	tb/tb_widget_value.cpp
	tb/tb_widgets.cpp
	tb/tb_widgets_common.cpp
	tb/tb_widgets_listener.cpp
	tb/tb_widgets_reader.cpp
	tb/tb_window.cpp
	tb/tb_bitmap_fragment.h
	tb/tb_color.h
	tb/tb_config.h
	tb/tb_core.h
	tb/tb_debug.h
	tb/tb_dimension.h
	tb/tb_editfield.h
	tb/tb_font_desc.h
	tb/tb_font_renderer.h
	tb/tb_geometry.h
	tb/tb_hash.h
	tb/tb_hashtable.h
	tb/tb_id.h
	tb/tb_inline_select.h
	tb/tb_language.h
	tb/tb_layout.h
	tb/tb_linklist.h
	tb/tb_list.h
	tb/tb_menu_window.h
	tb/tb_message_window.h
	tb/tb_msg.h
	tb/tb_node_ref_tree.h
	tb/tb_node_tree.h
	tb/tb_object.h
	tb/tb_popup_window.h
	tb/tb_renderer.h
	tb/tb_scroll_container.h
	tb/tb_scroller.h
	tb/tb_select.h
	tb/tb_select_item.h
	tb/tb_skin.h
	tb/tb_skin_util.h
	tb/tb_sort.h
	tb/tb_str.h
	tb/tb_style_edit.h
	tb/tb_style_edit_content.h
	tb/tb_system.h
	tb/tb_tab_container.h
	tb/tb_tempbuffer.h
	tb/tb_toggle_container.h
	tb/tb_types.h
	tb/tb_value.h
	tb/tb_widget_skin_condition_context.h
	tb/tb_widget_value.h
	tb/tb_widgets.h
	tb/tb_widgets_common.h
	tb/tb_widgets_listener.h
	tb/tb_widgets_reader.h
	tb/tb_window.h

	UIApp.cpp UIApp.h
	TurboBadger.h
	Window.cpp Window.h
	Widget.h
	FontUtil.h
	FileDialogWindow.h FileDialogWindow.cpp
	WaitingMessage.h WaitingMessage.cpp
	Console.cpp Console.h
	ui_renderer_gl.cpp ui_renderer_gl.h
	ui_widgets.cpp ui_widgets.h
)

set(IMAGES
	shared/ui/skin/arrow_down
	shared/ui/skin/arrow_left
	shared/ui/skin/arrow_right
	shared/ui/skin/arrow_up
	shared/ui/skin/button_flat_outline
	shared/ui/skin/button_flat_pressed
	shared/ui/skin/button_grouped_x_first_down
	shared/ui/skin/button_grouped_x_first_up
	shared/ui/skin/button_grouped_x_last_down
	shared/ui/skin/button_grouped_x_last_up
	shared/ui/skin/button_grouped_x_middle_down
	shared/ui/skin/button_grouped_x_middle_up
	shared/ui/skin/button
	shared/ui/skin/button_pressed
	shared/ui/skin/checkbox_mark
	shared/ui/skin/checkbox
	shared/ui/skin/checkbox_pressed
	shared/ui/skin/container
	shared/ui/skin/editfield
	shared/ui/skin/fadeout_x
	shared/ui/skin/fadeout_y
	shared/ui/skin/filedialog-dir
	shared/ui/skin/filedialog-file
	shared/ui/skin/focus_r4
	shared/ui/skin/focus_tabbutton_left
	shared/ui/skin/focus_tabbutton_top
	shared/ui/skin/item_hover
	shared/ui/skin/item_selected
	shared/ui/skin/item_separator_x
	shared/ui/skin/item_separator_y
	shared/ui/skin/progress_spinner_strip
	shared/ui/skin/radio_mark
	shared/ui/skin/radio
	shared/ui/skin/radio_pressed
	shared/ui/skin/remove
	shared/ui/skin/resizer
	shared/ui/skin/scroll_bg_x
	shared/ui/skin/scroll_bg_y
	shared/ui/skin/scroll_fg_x
	shared/ui/skin/scroll_fg_y
	shared/ui/skin/search
	shared/ui/skin/section_container
	shared/ui/skin/selection
	shared/ui/skin/slider_bg_x
	shared/ui/skin/slider_bg_y
	shared/ui/skin/slider_handle
	shared/ui/skin/tab_button_bottom_active
	shared/ui/skin/tab_button_bottom_inactive
	shared/ui/skin/tab_button_left_active
	shared/ui/skin/tab_button_left_inactive
	shared/ui/skin/tab_button_right_active
	shared/ui/skin/tab_button_right_inactive
	shared/ui/skin/tab_button_top_active
	shared/ui/skin/tab_button_top_inactive
	shared/ui/skin/toggle_section_icon_down
	shared/ui/skin/toggle_section_icon_middle
	shared/ui/skin/toggle_section_icon_up
	shared/ui/skin/window_active
	shared/ui/skin/window_close
	shared/ui/skin/window_close_pressed
	shared/ui/skin/window_mover_bg_tile
	shared/ui/skin/window_mover_overlay
	shared/ui/skin/window
)

set(FILES
	shared/ui/skin/skin.tb.txt
	shared/ui/font/font_14.png
	shared/ui/font/font_28.png
	shared/ui/font/font.tb.txt
	shared/ui/font/DejaVuSansMono.ttf
	shared/ui/window/filedialog_dir.tb.txt
	shared/ui/window/filedialog_file.tb.txt
	shared/ui/window/filedialog.tb.txt
)

foreach(IMAGE ${IMAGES})
	list(APPEND FILES ${IMAGE}.png)
endforeach()
set(DPIS "@288")
foreach(DPI ${DPIS})
	foreach(IMAGE ${IMAGES})
		list(APPEND FILES ${IMAGE}${DPI}.png)
	endforeach()
endforeach()

if (WINDOWS)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
	add_definitions(-DTB_USE_CURRENT_DIRECTORY)
elseif (LINUX)
elseif (DARWIN)
	add_definitions(-DMACOSX)
	add_definitions(-DTB_USE_CURRENT_DIRECTORY)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES app render)
target_include_directories(${LIB} PUBLIC tb)
