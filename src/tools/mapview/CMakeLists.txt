project(mapview)
set(SRCS
	MapView.h MapView.cpp
)
set(FILES
	shared/worldparams.lua
	shared/biomes.lua
	mapview/mapview-keybindings.cfg
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui voxelrender)
