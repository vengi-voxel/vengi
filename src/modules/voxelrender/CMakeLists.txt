set(LIB voxelrender)
set(SRCS
	SceneGraphRenderer.cpp SceneGraphRenderer.h
	Shadow.h Shadow.cpp
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	ShaderAttribute.h
	ImageGenerator.h ImageGenerator.cpp
)
set(SHADERS voxel)
set(SRCS_SHADERS
	shaders/_voxelflags.glsl
)
foreach (SHADER ${SHADERS})
	list(APPEND SRCS_SHADERS "shaders/${SHADER}.vert")
	list(APPEND SRCS_SHADERS "shaders/${SHADER}.frag")
endforeach()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES render voxelformat)
generate_shaders(${LIB} ${SHADERS})

set(TEST_SRCS
	tests/RawVolumeRendererTest.cpp
	tests/VoxelRenderShaderTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
