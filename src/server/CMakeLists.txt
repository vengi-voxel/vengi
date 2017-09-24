project(server)
set(SRCS
	Server.cpp Server.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES backend)
check_lua_files(server behaviourtrees.lua attributes.lua events.lua)
