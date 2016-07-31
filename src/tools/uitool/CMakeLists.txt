project(uitool)
set(SRCS
	UITool.h UITool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
target_link_libraries(${PROJECT_NAME} ui)
