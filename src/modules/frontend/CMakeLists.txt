set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	Movement.h
	MeshShader.h MeshShader.cpp
	ColorShader.h ColorShader.cpp
	WorldRenderer.h WorldRenderer.cpp
	WorldShader.h WorldShader.cpp
)
set(LIB frontend)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} ui network voxel cooldown)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/MaterialTest.cpp
)
gtest_suite_deps(tests ${LIB} frontend image)
