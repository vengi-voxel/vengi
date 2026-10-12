set(LIB voxelpathtracer)
set(SRCS
	PathTracer.cpp PathTracer.h
	PathTracerScene.cpp PathTracerScene.h
	PathTracerState.h
	GpuSceneData.cpp GpuSceneData.h
)
set(SHADERS
	voxelpathtracer
	voxelpathtracer_display
	voxelpathtracer_bloom
)
set(SRCS_SHADERS
	shaders/_dda.glsl
	shaders/_pathtracershared.glsl
	shaders/_gpuscene.inc
	shaders/_struct.glsl
	shaders/_sky.glsl
	shaders/_nee.glsl
	shaders/_camera.glsl
	shaders/voxelpathtracer.vert
	shaders/voxelpathtracer.frag
	shaders/voxelpathtracer_display.vert
	shaders/voxelpathtracer_display.frag
	shaders/voxelpathtracer_bloom.vert
	shaders/voxelpathtracer_bloom.frag
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES voxelrender voxelutil image)
engine_generate_shaders(${LIB} ${SHADERS})

set(TEST_SRCS
	tests/PathTracerTest.cpp
)
set(TEST_FILES
	tests/hmec.vxl
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app voxelformat voxelrender)
gtest_suite_end(tests-${LIB})

if (USE_BENCHMARKS)
	engine_add_executable(TARGET benchmarks-${LIB} SRCS benchmarks/PathTracerBenchmark.cpp NOINSTALL)
	engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
endif()
