set(SRCS
	cl/CL.h
	cl/CLCompute.cpp
	cl/CLTypes.h
	cl/CLSymbol.c cl/CLSymbol.h

	Types.h
	Compute.h
	Shader.h Shader.cpp
)
set(LIB compute)
find_package(OpenCL)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core util)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
set(OPENCL_LIBRARY ${OpenCL_LIBRARY})
if (OpenCL_INCLUDE_DIRS)
	target_include_directories(${LIB} PUBLIC ${OpenCL_INCLUDE_DIRS})
endif()

set(TEST_SRCS
	tests/ComputeShaderTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
generate_compute_shaders(tests test)
