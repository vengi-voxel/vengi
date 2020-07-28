set(SRCS
	Texture.h Texture.cpp
	Types.h
	Trace.h Trace.cpp
	Compute.h
	Shader.h Shader.cpp
	TextureConfig.h TextureConfig.cpp
)
# its important to not link against opencl here - we are loading the icd lib at runtime
set(LIB compute)
find_package(OpenCL)
if (OpenCL_INCLUDE_DIRS)
	list(APPEND SRCS
		cl/CL.h
		cl/CLCompute.cpp cl/CLCompute.h
		cl/CLMapping.h
		cl/CLTypes.h
		cl/CLSymbol.c cl/CLSymbol.h
	)
	set(OPENCL_LIBRARY ${OpenCL_LIBRARY})
	set(HAVE_OPENCL 1 CACHE FORCE "")
else()
	set(HAVE_OPENCL 0 CACHE FORCE "")
	list(APPEND SRCS dummy/DummyCompute.cpp)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES util)
if (APPLE)
	target_link_libraries(${LIB} PRIVATE "-framework OpenCL")
endif()
if (OpenCL_INCLUDE_DIRS)
	target_include_directories(${LIB} PRIVATE ${OpenCL_INCLUDE_DIRS})
endif()

if (TESTS)
	set(TEST_SRCS
		tests/ComputeShaderTest.cpp
	)

	gtest_suite_sources(tests ${TEST_SRCS})
	gtest_suite_deps(tests ${LIB})
	generate_compute_shaders(tests test)
endif()
