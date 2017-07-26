set(SRCS
	cl/CL.h
	cl/CLCompute.cpp
	cl/CLTypes.h
	cl/CLMapping.h

	Types.h
	Compute.h
	Shader.h Shader.cpp
)
set(LIB compute)
find_package(OpenCL REQUIRED)
add_library(${LIB} ${SRCS})
# TODO: don't link against OpenCL - dyload it
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core OpenCL ${OPENCL_LIBRARY})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC ${OPENCL_INCLUDE_DIRS})

set(TEST_SRCS
	tests/ComputeShaderTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
