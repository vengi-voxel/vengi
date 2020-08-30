project(testhttpserver)
set(SRCS
	TestHttpServer.h TestHttpServer.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES console http)
