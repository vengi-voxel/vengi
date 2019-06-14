project(testturbobadger)
set(SRCS
	Demo01.cpp Demo.h
	ListWindow.cpp ListWindow.h
	ResourceEditWindow.cpp ResourceEditWindow.h
	TestTurbobadger.h TestTurbobadger.cpp
)
set(FILES
	testturbobadger/demo01/ui_resources/test_textwindow.tb.txt
	testturbobadger/demo01/ui_resources/test_tabcontainer01.tb.txt
	testturbobadger/demo01/ui_resources/test_scrollcontainer.tb.txt
	testturbobadger/demo01/ui_resources/test_select_advanced.tb.txt
	testturbobadger/demo01/ui_resources/test_layout02.tb.txt
	testturbobadger/demo01/ui_resources/test_animations.tb.txt
	testturbobadger/demo01/ui_resources/test_ui.tb.txt
	testturbobadger/demo01/ui_resources/test_layout01.tb.txt
	testturbobadger/demo01/ui_resources/test_connections.tb.txt
	testturbobadger/demo01/ui_resources/test_list_item.tb.txt
	testturbobadger/demo01/ui_resources/test_select.tb.txt
	testturbobadger/demo01/ui_resources/test_radio_checkbox.tb.txt
	testturbobadger/demo01/ui_resources/resource_edit_test.tb.txt
	testturbobadger/demo01/ui_resources/test_toggle_containers.tb.txt
	testturbobadger/demo01/ui_resources/test_image_widget.tb.txt
	testturbobadger/demo01/ui_resources/resource_edit_window.tb.txt
	testturbobadger/demo01/ui_resources/test_scroller_snap.tb.txt
	testturbobadger/demo01/ui_resources/test_skin_conditions02.tb.txt
	testturbobadger/demo01/ui_resources/test_skin_conditions01.tb.txt
	testturbobadger/demo01/ui_resources/test_batching01.tb.txt
	testturbobadger/demo01/ui_resources/test_layout03.tb.txt
	testturbobadger/demo01/language/lng_en.tb.txt
	testturbobadger/demo01/language/lng_sv.tb.txt
	testturbobadger/demo01/images/image_3.png
	testturbobadger/demo01/images/image_1.png
	testturbobadger/demo01/images/image_6.png
	testturbobadger/demo01/images/image_2.png
	testturbobadger/demo01/images/image_8.png
	testturbobadger/demo01/images/image_5.png
	testturbobadger/demo01/images/image_7.png
	testturbobadger/demo01/images/image_4.png
	testturbobadger/demo01/images/image_9.png
	testturbobadger/demo01/skin/star_gray.png
	testturbobadger/demo01/skin/star_gold@192.png
	testturbobadger/demo01/skin/star_gold@384.png
	testturbobadger/demo01/skin/icon128.png
	testturbobadger/demo01/skin/star_gold.png
	testturbobadger/demo01/skin/star_gray@384.png
	testturbobadger/demo01/skin/remove.png
	testturbobadger/demo01/skin/special_button_pressed.png
	testturbobadger/demo01/skin/special_button.png
	testturbobadger/demo01/skin/icon16.png
	testturbobadger/demo01/skin/testturbobadger-skin.tb.txt
	testturbobadger/demo01/skin/star_gold@288.png
	testturbobadger/demo01/skin/image_frame.png
	testturbobadger/demo01/skin/bg_tile.png
	testturbobadger/demo01/skin/star_gray@288.png
	testturbobadger/demo01/skin/icon48.png
	testturbobadger/demo01/skin/focus_r8.png
	testturbobadger/demo01/skin/star_gray@192.png
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger)
