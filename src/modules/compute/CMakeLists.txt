set(SRCS
	cl/CL.h
	cl/CLCompute.cpp
	cl/CLMapping.h
	cl/CLTypes.h
	cl/CLSymbol.c cl/CLSymbol.h

	Texture.h Texture.cpp
	Types.h
	Compute.h
	Shader.h Shader.cpp
	TextureConfig.h TextureConfig.cpp
)
set(LIB compute)
find_package(OpenCL)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES util)
set(OPENCL_LIBRARY ${OpenCL_LIBRARY})
if (OpenCL_INCLUDE_DIRS)
	target_include_directories(${LIB} PRIVATE ${OpenCL_INCLUDE_DIRS})
endif()

set(TEST_SRCS
	tests/ComputeShaderTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
generate_compute_shaders(tests test)
