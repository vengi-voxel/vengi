project(testimgui)
set(SRCS
	TestIMGUI.h TestIMGUI.cpp

	imgui_node_graph_test.cpp imgui_node_graph_test.h
	imgui_demo.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} imgui)
