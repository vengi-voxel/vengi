set(LIB palette)
set(SRCS
	private/PaletteFormat.cpp private/PaletteFormat.h

	private/AdobeColorSpace.h
	private/ACBPalette.cpp private/ACBPalette.h
	private/ASEPalette.cpp private/ASEPalette.h
	private/AVMTPalette.cpp private/AVMTPalette.h
	private/AVMTHelper.cpp private/AVMTHelper.h
	private/CSVPalette.cpp private/CSVPalette.h
	private/GimpPalette.cpp private/GimpPalette.h
	private/JASCPalette.cpp private/JASCPalette.h
	private/PaintNetPalette.cpp private/PaintNetPalette.h
	private/PhotoshopPalette.cpp private/PhotoshopPalette.h
	private/PNGPalette.cpp private/PNGPalette.h
	private/QBCLPalette.cpp private/QBCLPalette.h
	private/RGBPalette.cpp private/RGBPalette.h
	private/PixeloramaPalette.cpp private/PixeloramaPalette.h
	private/VPLPalette.cpp private/VPLPalette.h

	FormatConfig.cpp FormatConfig.h
	Material.cpp Material.h
	NormalPalette.cpp NormalPalette.h
	NormalPaletteLookup.h

	PaletteFormatDescription.cpp PaletteFormatDescription.h

	PaletteCache.cpp PaletteCache.h

	PaletteView.h PaletteView.cpp
	Palette.h Palette.cpp
	PaletteLookup.h PaletteLookup.cpp
	PaletteCompleter.h
	RGBABuffer.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES util image)

set(TEST_SRCS
	tests/NormalPaletteTest.cpp
	tests/PaletteTest.cpp
)

set(TEST_FILES
	tests/test-palette-in.png
	tests/paint.net.txt
	tests/test.aco
	tests/test.vpl
	tests/pixelorama.json
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${FILES} ${TEST_FILES})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/PaletteBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
