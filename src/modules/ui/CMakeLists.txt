set(SRCS
	UIApp.cpp UIApp.h
	TurboBadger.h
	Window.cpp Window.h
	Console.cpp Console.h
	KeybindingParser.h
	ui_renderer_gl.cpp ui_renderer_gl.h
)
set(LIB ui)
add_library(${LIB} ${SRCS})
check_glsl_files(${LIB} ui)
target_link_libraries(${LIB} core io video turbobadger)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/KeybindingParserTest.cpp
)
gtest_suite_deps(tests ${LIB})
