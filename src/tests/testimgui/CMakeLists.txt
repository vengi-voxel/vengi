project(testimgui)
set(SRCS
	TestIMGUI.h TestIMGUI.cpp

	Demo.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui)
