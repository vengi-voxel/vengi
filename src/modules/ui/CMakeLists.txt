fips_begin_module(ui)
	fips_files(
		UIApp.cpp UIApp.h
		TurboBadger.h
		Window.cpp Window.h
		KeybindingParser.h
		ui_renderer_gl.cpp ui_renderer_gl.h
	)
	find_package(OpenGL)
	find_package(SDL2 REQUIRED)
	fips_include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
	fips_deps(core io video turbobadger ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
fips_end_module()

gtest_suite_files(tests tests/KeybindingParserTest.cpp)
gtest_suite_deps(tests ui)
