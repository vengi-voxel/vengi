project(testluaui)
set(SRCS
	TestLUAUI.h TestLUAUI.cpp
)
set(LUA_SRCS
	ui/${PROJECT_NAME}.lua
)
set(FILES
	voxel/assets/north-dir.qb
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED FILES ${FILES} LUA_SRCS ${LUA_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES nuklear)
