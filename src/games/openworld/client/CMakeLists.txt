project(client)
add_subdirectory(modules)

set(SRCS
	Client.h Client.cpp
	ClientLUA.h ClientLUA.cpp
)

set(LUA_SRCS
	ui/shared.lua
	ui/style.lua
	ui/window/auth_failed.lua
	ui/window/disconnect.lua
	ui/window/hud.lua
	ui/window/main.lua
	ui/window/options.lua
	ui/window/popup.lua
	ui/window/style.lua
	ui/window/validation_failed.lua
	ui/${PROJECT_NAME}.lua
)

set(FILES
	client/client-keybindings.cfg

	shared/font.ttf
	shared/biomes.lua
	shared/worldparams.lua
)
set(DEPENDENCIES nuklear voxelworldrender client-network client-cooldown client-voxel)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} LUA_SRCS ${LUA_SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES ${DEPENDENCIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} modules)
