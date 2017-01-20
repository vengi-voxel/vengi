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
target_link_libraries(${PROJECT_NAME} frontend util restclient-cpp)
check_ui_files(${PROJECT_NAME})

gtest_suite_files(tests
	tests/ClientUITest.cpp
)
