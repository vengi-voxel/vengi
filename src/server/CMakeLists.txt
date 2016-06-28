project(server)
set(SRCS
	Server.cpp Server.h
	ServerModule.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
target_link_libraries(${PROJECT_NAME} backend)
check_lua_files(server behaviourtrees.lua)
check_lua_files(server attributes.lua)
