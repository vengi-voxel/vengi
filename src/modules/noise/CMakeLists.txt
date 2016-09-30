set(SRCS
	SimplexNoise.h SimplexNoise.cpp
)
set(LIB noise)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
#set(MARCH native)
set(MARCH generic)
# http://christian-seiler.de/projekte/fpmath/
if (NOT MSVC)
	target_compile_options(${LIB} PRIVATE -O3 PRIVATE -mtune=${MARCH} PRIVATE -ffloat-store)
else()
	# TODO: MSVC
endif()

gtest_suite_files(tests
	tests/SimplexNoiseTest.cpp
)
gtest_suite_deps(tests ${LIB} image)
