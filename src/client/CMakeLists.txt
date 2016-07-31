project(client)
set(SRCS
	Client.h Client.cpp
	ClientModule.h
	network/AuthFailedHandler.h
	network/ClientNetworkModule.h
	network/NpcSpawnHandler.h
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
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util)
check_ui_files(${PROJECT_NAME})

gtest_suite_files(tests
	tests/ClientUITest.cpp
)
