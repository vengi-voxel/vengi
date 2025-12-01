set(EXTERNAL_SRCS
	external/jo_mpeg.cpp external/jo_mpeg.h
	ImageExt.cpp
)

set(SRCS
	${EXTERNAL_SRCS}
	Image.cpp Image.h
	AVI.cpp AVI.h
	CaptureTool.cpp CaptureTool.h
)
set(LIB image)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES util stb_image)
if (NOT MSVC)
	target_compile_options(${LIB} PRIVATE -Wno-unused-function -Wno-missing-field-initializers)
endif()
foreach (src ${EXTERNAL_SRCS})
	engine_source_optimize(${src})
endforeach()

# TODO: not included as local libs yet
if (NOT USE_LIBS_FORCE_LOCAL)
	find_package(JPEG)
	if (JPEG_FOUND)
		target_compile_definitions(${LIB} PRIVATE -DUSE_LIBJPEG)
		target_link_libraries(${LIB} PRIVATE JPEG::JPEG)
	endif()
	find_package(PNG)
	if (PNG_FOUND)
		target_compile_definitions(${LIB} PRIVATE -DUSE_LIBPNG)
		target_link_libraries(${LIB} PRIVATE PNG::PNG)
	endif()
else()
	set(JPEG_FOUND 0)
	set(PNG_FOUND 0)
endif()
set(USE_LIBJPEG ${JPEG_FOUND} CACHE BOOL "Use libjpeg" FORCE)
set(USE_LIBPNG ${PNG_FOUND} CACHE BOOL "Use libpng" FORCE)

set(TEST_SRCS
	tests/ImageTest.cpp
	tests/AVITest.cpp
	tests/MPEG2Test.cpp
	tests/CaptureToolTest.cpp
)

set(TEST_FILES
	tests/test-palette-in.png
	tests/benchmark-caveexpress.jpg
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/ImageBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} FILES ${TEST_FILES} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
