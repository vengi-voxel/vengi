set(LIB voxedit-ui)

set(SRCS
	LSystemPanel.h LSystemPanel.cpp
	NoisePanel.h NoisePanel.cpp
	Viewport.h Viewport.cpp
	VoxEditWindow.h VoxEditWindow.cpp
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES imgui voxedit-util)
