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

begin_googleunittest(ui)
	fips_dir(tests)
	fips_files(
		KeybindingParserTest.cpp
	)
	find_package(OpenGL)
	find_package(SDL2 REQUIRED)
	fips_include_directories(. ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
	fips_deps(core io video turbobadger ui ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
end_googleunittest()
