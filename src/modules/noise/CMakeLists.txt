set(SRCS
	SimplexNoise.h SimplexNoise.cpp
)
set(LIB noise)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_compile_options(${LIB} PRIVATE -march=native PRIVATE -O3)

gtest_suite_files(tests
	tests/SimplexNoiseTest.cpp
)
gtest_suite_deps(tests ${LIB} image)
