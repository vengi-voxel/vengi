project(mapedit)
set(SRCS
	MapEdit.h MapEdit.cpp
)
set(FILES
	shared/worldparams.lua
	testmesh/chr_skelett.fbx
	testmesh/chr_skelett2.png
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui voxelrender)
