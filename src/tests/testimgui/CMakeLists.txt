project(testimgui)
set(SRCS
	TestIMGUI.h TestIMGUI.cpp

	NodeGraph.cpp NodeGraph.h
	Demo.cpp

	nodes/ColorNode.h
	nodes/CombineNode.h
	nodes/CommentNode.h
	nodes/Node.h
	nodes/NoiseNode.h
	nodes/OutputNode.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} imgui)
