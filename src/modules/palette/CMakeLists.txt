set(LIB palette)
set(SRCS
	private/PaletteFormat.cpp private/PaletteFormat.h
	private/ASEPalette.cpp private/ASEPalette.h
	private/CSVPalette.cpp private/CSVPalette.h
	private/GimpPalette.cpp private/GimpPalette.h
	private/JASCPalette.cpp private/JASCPalette.h
	private/PaintNetPalette.cpp private/PaintNetPalette.h
	private/PNGPalette.cpp private/PNGPalette.h
	private/QBCLPalette.cpp private/QBCLPalette.h
	private/RGBPalette.cpp private/RGBPalette.h

	Palette.h Palette.cpp
	PaletteLookup.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES util image http)

set(TEST_SRCS
	tests/PaletteTest.cpp
)

set(TEST_FILES
	tests/test-palette-in.png
	tests/paint.net.txt
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${FILES} ${TEST_FILES})
gtest_suite_end(tests-${LIB})
