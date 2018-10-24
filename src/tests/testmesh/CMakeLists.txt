project(testmesh)
set(SRCS
	TestMesh.cpp
)
set(FILES
	testmesh/chr_skelett.fbx
	testmesh/chr_skelett2.png
	testmesh/big-buck-bunny.obj
)
engine_add_executable(TARGET ${PROJECT_NAME} FILES ${FILES} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
