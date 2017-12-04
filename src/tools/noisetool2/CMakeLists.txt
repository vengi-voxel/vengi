project(noisetool2)
set(SRCS
	NoiseTool2.h NoiseTool2.cpp

	NodeGraph.cpp NodeGraph.h

	nodes/ResultNodes.h
	nodes/GraphNode.h nodes/GraphNode.cpp
	nodes/NNode.h nodes/NNode.cpp
	nodes/VolumeNode.h nodes/VolumeNode.cpp
	nodes/ConstantNode.h nodes/ConstantNode.cpp
	nodes/NoiseNode.h nodes/NoiseNode.cpp
	nodes/NormalizeNode.h nodes/NormalizeNode.cpp
	nodes/RGBANode.h nodes/RGBANode.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui noise image voxel frontend collection)
