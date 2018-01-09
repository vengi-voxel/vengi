project(uitool)
set(SRCS
	UITool.h UITool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger)
