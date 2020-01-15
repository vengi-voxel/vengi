project(client)
set(SRCS
	Client.h Client.cpp

	voxel/ClientPager.h
	voxel/ClientPager.cpp

	network/AttribUpdateHandler.h
	network/AuthFailedHandler.h
	network/EntitySpawnHandler.h
	network/InitHandler.cpp
	network/InitHandler.h
	network/UserSpawnHandler.h
	network/EntityUpdateHandler.h
	network/EntityRemoveHandler.h
	network/VarUpdateHandler.h

	ui/LoginWindow.h
	ui/SignupWindow.h
	ui/HudWindow.h
	ui/AuthFailedWindow.h
	ui/DisconnectWindow.h
	ui/LostPasswordWindow.h
)

set(UI_WINDOWS
	client/ui/window/client-authfailed.tb.txt
	client/ui/window/client-disconnect.tb.txt
	client/ui/window/client-hud.tb.txt
	client/ui/window/client-login.tb.txt
	client/ui/window/client-lostpassword.tb.txt
	client/ui/window/client-signup.tb.txt
)

set(FILES
	${UI_WINDOWS}
	client/ui/lang/en.tb.txt
	client/client-keybindings.cfg

	shared/font.ttf
	shared/biomes.lua
	shared/worldparams.lua
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger animation voxelfont voxelrender util stock)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
check_ui_turbobadger(${PROJECT_NAME} ${UI_WINDOWS})
