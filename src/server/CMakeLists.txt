project(server)
set(SRCS
	Server.cpp Server.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES backend)
set(LUA_SRCS
	behaviourtrees.lua
	attributes.lua
	events.lua
	world.lua
	stock.lua
	map/map001.lua
)
check_lua_files(${PROJECT_NAME} ${LUA_SRCS})
