set(SRCS
	Image.cpp Image.h
	AVI.cpp AVI.h
	AVIRecorder.cpp AVIRecorder.h
)
set(LIB image)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES io util stb_image)
if (NOT MSVC)
	target_compile_options(${LIB} PRIVATE -Wno-unused-function -Wno-missing-field-initializers)
endif()

set(TEST_SRCS
	tests/ImageTest.cpp
	tests/AVITest.cpp
)

set(TEST_FILES
	tests/test-palette-in.png
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
