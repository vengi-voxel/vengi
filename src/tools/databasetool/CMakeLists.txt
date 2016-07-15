project(databasetool)
set(SRCS
	DatabaseTool.h DatabaseTool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
target_link_libraries(${PROJECT_NAME} persistence)
