project(testimgui)
set(SRCS
	TestIMGUI.h TestIMGUI.cpp

	NodeGraph.cpp NodeGraph.h
	Demo.cpp

	nodes/CombineNode.h
	nodes/NNode.h nodes/NNode.cpp
	nodes/NoiseNode.h nodes/NoiseNode.cpp
	nodes/NormalizeNode.h nodes/NormalizeNode.cpp
	nodes/RGBANode.h nodes/RGBANode.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} imgui noise image)
