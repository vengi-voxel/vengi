project(mapview)
set(SRCS
	MapView.h MapView.cpp
)
set(FILES
	shared/worldparams.lua
	shared/biomes.lua
	mapview/mapview-keybindings.cfg
)
set(LUA_ATTRIBUTES
	attributes.lua
)
set(LUA_SRCS ${LUA_ATTRIBUTES})

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} LUA_SRCS ${LUA_SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui voxelrender attrib testcore)
