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
set(LIB ui)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} ui)
target_link_libraries(${LIB} core io video turbobadger)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/ConsoleTest.cpp
	tests/UITest.h
)
gtest_suite_deps(tests ${LIB})
