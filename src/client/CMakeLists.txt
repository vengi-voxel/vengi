project(client)
set(SRCS
	Client.h Client.cpp

	voxel/ClientPager.h
	voxel/ClientPager.cpp

	network/AttribUpdateHandler.h
	network/AuthFailedHandler.h
	network/EntitySpawnHandler.h
	network/UserInfoHandler.h
	network/UserSpawnHandler.h
	network/EntityUpdateHandler.h
	network/EntityRemoveHandler.h
	network/VarUpdateHandler.h
)

set(LUA_SRCS
	ui/shared.lua
	ui/style.lua
	ui/window/auth_failed.lua
	ui/window/disconnect.lua
	ui/window/hud.lua
	ui/window/login.lua
	ui/window/main.lua
	ui/window/options.lua
	ui/window/popup.lua
	ui/window/style.lua
	ui/${PROJECT_NAME}.lua
)

set(FILES
	client/client-keybindings.cfg

	shared/font.ttf
	shared/biomes.lua
	shared/worldparams.lua
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} LUA_SRCS ${LUA_SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES nuklear animation voxelrender util stock http)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
