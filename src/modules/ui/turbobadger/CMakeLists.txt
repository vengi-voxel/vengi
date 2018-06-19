set(LIB turbobadger)
set(SRCS
	UIApp.cpp UIApp.h
	TurboBadger.h
	Window.cpp Window.h
	Widget.h
	FontUtil.h
	WaitingMessage.h WaitingMessage.cpp
	Console.cpp Console.h
	ui_renderer_gl.cpp ui_renderer_gl.h
	ui_widgets.cpp ui_widgets.h
)
set(FILES
	shared/ui/font/font_14.png
	shared/ui/font/font_28.png
	shared/ui/font/font.tb.txt
	shared/ui/skin/arrow_down.png
	shared/ui/skin/arrow_left.png
	shared/ui/skin/arrow_right.png
	shared/ui/skin/arrow_up.png
	shared/ui/skin/button_flat_outline.png
	shared/ui/skin/button_flat_pressed.png
	shared/ui/skin/button_grouped_x_first_down.png
	shared/ui/skin/button_grouped_x_first_up.png
	shared/ui/skin/button_grouped_x_last_down.png
	shared/ui/skin/button_grouped_x_last_up.png
	shared/ui/skin/button_grouped_x_middle_down.png
	shared/ui/skin/button_grouped_x_middle_up.png
	shared/ui/skin/button.png
	shared/ui/skin/button_pressed.png
	shared/ui/skin/checkbox_mark.png
	shared/ui/skin/checkbox.png
	shared/ui/skin/checkbox_pressed.png
	shared/ui/skin/container.png
	shared/ui/skin/editfield.png
	shared/ui/skin/fadeout_x.png
	shared/ui/skin/fadeout_y.png
	shared/ui/skin/focus_r4.png
	shared/ui/skin/focus_tabbutton_left.png
	shared/ui/skin/focus_tabbutton_top.png
	shared/ui/skin/item_hover.png
	shared/ui/skin/item_selected.png
	shared/ui/skin/item_separator_x.png
	shared/ui/skin/item_separator_y.png
	shared/ui/skin/progress_spinner_strip.png
	shared/ui/skin/radio_mark.png
	shared/ui/skin/radio.png
	shared/ui/skin/radio_pressed.png
	shared/ui/skin/remove.png
	shared/ui/skin/resizer.png
	shared/ui/skin/scroll_bg_x.png
	shared/ui/skin/scroll_bg_y.png
	shared/ui/skin/scroll_fg_x.png
	shared/ui/skin/scroll_fg_y.png
	shared/ui/skin/search.png
	shared/ui/skin/section_container.png
	shared/ui/skin/selection.png
	shared/ui/skin/skin.tb.txt
	shared/ui/skin/slider_bg_x.png
	shared/ui/skin/slider_bg_y.png
	shared/ui/skin/slider_handle.png
	shared/ui/skin/tab_button_bottom_active.png
	shared/ui/skin/tab_button_bottom_inactive.png
	shared/ui/skin/tab_button_left_active.png
	shared/ui/skin/tab_button_left_inactive.png
	shared/ui/skin/tab_button_right_active.png
	shared/ui/skin/tab_button_right_inactive.png
	shared/ui/skin/tab_button_top_active.png
	shared/ui/skin/tab_button_top_inactive.png
	shared/ui/skin/toggle_section_icon_down.png
	shared/ui/skin/toggle_section_icon_middle.png
	shared/ui/skin/toggle_section_icon_up.png
	shared/ui/skin/window_active.png
	shared/ui/skin/window_close.png
	shared/ui/skin/window_close_pressed.png
	shared/ui/skin/window_mover_bg_tile.png
	shared/ui/skin/window_mover_overlay.png
	shared/ui/skin/window.png
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES core io render util libturbobadger)

gtest_suite_sources(tests
	tests/ConsoleTest.cpp
	tests/UITest.h
)
gtest_suite_deps(tests ${LIB})
