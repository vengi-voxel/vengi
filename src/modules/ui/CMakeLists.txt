fips_begin_module(ui)
	fips_files(
		UIApp.cpp UIApp.h
		TurboBadger.h
		Window.cpp Window.h
		KeybindingParser.h
		ui_renderer_gl.cpp ui_renderer_gl.h
	)
	fips_deps(core io video turbobadger)
fips_end_module()

gtest_suite_files(tests tests/KeybindingParserTest.cpp)
gtest_suite_deps(tests ui)
