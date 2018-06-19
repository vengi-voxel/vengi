project(testluaui)
set(SRCS
	TestLUAUI.h TestLUAUI.cpp
)
set(LUA_SRCS
	ui/${PROJECT_NAME}.lua
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED LUA_SRCS ${LUA_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES nuklear)
