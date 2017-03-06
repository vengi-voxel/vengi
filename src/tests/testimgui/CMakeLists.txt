project(testimgui)
set(SRCS
	TestIMGUI.h TestIMGUI.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} imgui)
