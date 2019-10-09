set(SRCS
	Mesh.cpp Mesh.h
	MeshPool.cpp MeshPool.h
	Vertex.h
)
set(SRCS_SHADERS
	shaders/mesh.vert shaders/mesh.frag
)

set(LIB mesh)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES video image util assimp)
generate_shaders(${LIB} mesh)

set(TEST_SRCS
	tests/MeshTest.cpp
)

gtest_suite_sources(tests ${TEST_SRC})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
