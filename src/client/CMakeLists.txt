project(client)
set(SRCS
	Client.h Client.cpp
	sauce/ClientModule.h
	sauce/ClientInjector.h
	network/AuthFailedHandler.h
	network/ClientNetworkModule.h
	network/NpcSpawnHandler.h
	network/SeedHandler.cpp
	network/SeedHandler.h
	network/UserSpawnHandler.h
	network/EntityUpdateHandler.h
	network/EntityRemoveHandler.h
	ui/LoginWindow.h
	ui/HudWindow.h
	ui/AuthFailedWindow.h
	ui/DisconnectWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util)
check_glsl_files(${PROJECT_NAME} world mesh ui color)
copy_data_files(${PROJECT_NAME})
