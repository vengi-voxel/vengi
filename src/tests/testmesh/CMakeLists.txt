project(testmesh)
set(SRCS
	TestMesh.cpp
)
set(FILES
	testmesh/mesh/chr_knight.fbx
	testmesh/mesh/chr_knight.png
)
engine_add_executable(TARGET ${PROJECT_NAME} FILES ${FILES} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
