project(testluaui)
set(SRCS
	TestLUAUI.h TestLUAUI.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES nuklear)
check_lua_files(${PROJECT_NAME} ui/${PROJECT_NAME}.lua)
