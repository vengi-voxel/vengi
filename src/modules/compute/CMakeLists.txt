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
find_package(OpenCL REQUIRED)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_compile_definitions(${LIB} PRIVATE -DOPENCL_LIBRARY="${OpenCL_LIBRARY}")
target_include_directories(${LIB} PUBLIC ${OpenCL_INCLUDE_DIRS})

set(TEST_SRCS
	tests/ComputeShaderTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
