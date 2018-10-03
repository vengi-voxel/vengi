project(client)
set(SRCS
	Client.h Client.cpp
	network/AttribUpdateHandler.h
	network/AuthFailedHandler.h
	network/EntitySpawnHandler.h
	network/SeedHandler.cpp
	network/SeedHandler.h
	network/UserSpawnHandler.h
	network/EntityUpdateHandler.h
	network/EntityRemoveHandler.h
	ui/LoginWindow.h
	ui/SignupWindow.h
	ui/HudWindow.h
	ui/AuthFailedWindow.h
	ui/DisconnectWindow.h
	ui/LostPasswordWindow.h
)
set(FILES
	client/mesh/animal_chicken.dae
	client/mesh/animal_chicken.png
	client/mesh/animal_rabbit.dae
	client/mesh/animal_rabbit.png
	client/mesh/animal_wolf.dae
	client/mesh/animal_wolf.png
	client/mesh/chr_fatkid.dae
	client/mesh/chr_fatkid.png
	client/ui/lang/en.tb.txt
	client/ui/window/client-authfailed.tb.txt
	client/ui/window/client-disconnect.tb.txt
	client/ui/window/client-hud.tb.txt
	client/ui/window/client-login.tb.txt
	client/ui/window/client-lostpassword.tb.txt
	client/ui/window/client-signup.tb.txt
	client/client-keybindings.cfg

	testmesh/chr_skelett.fbx
	testmesh/chr_skelett2.png

	shared/font.ttf
	shared/worldparams.lua
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger voxelfont voxelrender util)
check_ui_turbobadger(${PROJECT_NAME})
