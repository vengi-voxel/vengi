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
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger voxelfont voxelrender util)
check_ui_turbobadger(${PROJECT_NAME})

gtest_suite_files(tests
	tests/ClientUITest.cpp
)
